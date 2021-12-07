//
// Created by Mathieu Boisvert on 04-11-2021
//

#include <sys/time.h>

#include "ReseauGTFS.h"

using namespace std;

unsigned int getPoidsEntre2Coord(const double &vitesseDeMarche, const double &distanceMaxMarche,
                                 const Coordonnees &coordDepart,
                                 const Coordonnees &coordArrivee) {
    return (unsigned int) (((coordDepart - coordArrivee) / vitesseDeMarche) * 3600);
}

//! \brief Permet de valider qu'une station est présente dans les transferts
bool isStationPresenteDansTransfert(const Station &station,
                                    const vector<tuple<unsigned int, unsigned int, unsigned int>> &transferts) {
    // On vérifie si la station est présente comme from_station_id dans les transferts
    for (const auto &transfert : transferts) {
        if (get<0>(transfert) == station.getId()) {
            return true;
        }
    }
    return false;
}

//! \brief Permet de construire deux arrêt servant de noeud d'origine et de
//! destination
tuple<Arret::Ptr, Arret::Ptr> creerArretsOrigineDestination(const unsigned int stationIdOrigine,
                                                            const unsigned int stationIdDestination,
                                                            unsigned int nombreArcs) {
    Arret::Ptr arretOrigine = make_shared<Arret>(
            stationIdOrigine,
            Heure(1, 1, 1),
            Heure(9, 9, 9),
            0,
            "voyageIdOrigine"
    );

    Arret::Ptr arretDestination = make_shared<Arret>(
            stationIdDestination,
            Heure(1, 1, 1),
            Heure(9, 9, 9),
            0,
            "voyageIdDestination"
    );

    return tuple<Arret::Ptr, Arret::Ptr>(arretOrigine, arretDestination);
}

//! \brief Permet la récupération des arrêts des stations atteignable depuis le
//! noeud d'origine (au départ du graphe) \brief et dont le poids est plus petit
//! ou égale à distanceMaxMarche
multimap<Arret::Ptr, Coordonnees> getArretsAtteingnableAPiedDepuisOrigine(const Arret::Ptr &arretOrigine,
                                                                          const Coordonnees &coordArretOrigine,
                                                                          const double distanceMaxMarche,
                                                                          const map<unsigned int, Station> &stations,
                                                                          const map<string, Voyage> &voyages) {
    multimap<Arret::Ptr, Coordonnees> arretsAtteignable;

    // On filtre les stations par celles qui sont atteignable à pied
    for (const auto &itStations : stations) {
        const Station &station = itStations.second;

        if ((coordArretOrigine - station.getCoords()) <= distanceMaxMarche) {
            const multimap<Heure, Arret::Ptr> &arrets = station.getArrets();
            map<unsigned int, Arret::Ptr> arretsAvecLigneDifferente;

            for (const auto &itArrets : arrets) {
                Arret::Ptr arret = itArrets.second;

                unsigned int numLigne = voyages.at(arret->getVoyageId()).getLigne();
                auto it = arretsAvecLigneDifferente.find(numLigne);
                if (it != arretsAvecLigneDifferente.end()) {
                    if (it->second->getHeureArrivee() > arret->getHeureArrivee()) {
                        it->second = arret;
                    }
                } else {
                    arretsAvecLigneDifferente.insert({numLigne, arret});
                }

            }

            for (const auto &arret : arretsAvecLigneDifferente) {
                arretsAtteignable.insert({arret.second, station.getCoords()});
            }
        }
    }
    return arretsAtteignable;
}

//! \brief Permet la récupération des arrêts qui sont entre les stations et
//! l'arrêt destination (à la fin du graphe) \brief et dont le poids est plus
//! petit ou égale à distanceMaxMarche
multimap<Arret::Ptr, Coordonnees> getArretsEntreStationsEtDestination(const Arret::Ptr &arretDestination,
                                                                      const Coordonnees &coordArretDestination,
                                                                      const double distanceMaxMarche,
                                                                      const map<unsigned int, Station> &stations) {
    multimap<Arret::Ptr, Coordonnees> arretsAtteignable;

    for (const auto &itStations : stations) {
        const Station &station = itStations.second;
        if ((coordArretDestination - station.getCoords()) <= distanceMaxMarche) {
            const multimap<Heure, Arret::Ptr> &arrets = itStations.second.getArrets();
            for (const auto &itArrets : arrets) {
                arretsAtteignable.insert({itArrets.second, itStations.second.getCoords()});
            }
        }
    }

    return arretsAtteignable;
}

//! \brief Permet de récupérer les arcs attente
multimap<Arret::Ptr, Arret::Ptr> getArcsAttente(const unsigned int delaisMinArcAttente,
                                                const Station &station,
                                                const multimap<Heure, Arret::Ptr> &arretsDeLaStation,
                                                const map<string, Voyage> &voyages) {
    multimap<Arret::Ptr, Arret::Ptr> arcsAttente;

    // On boucle sur tous les arrêts
    for (const auto &itArretFrom: arretsDeLaStation) {
        Arret::Ptr arretFrom = itArretFrom.second;
        unsigned int arretFromNumeroLigne = voyages.find(arretFrom->getVoyageId())->second.getLigne();

        map<unsigned int, Arret::Ptr> arcsPossiblesVersArretTo;

        auto itArretTo = arretsDeLaStation.lower_bound(itArretFrom.first.add_secondes(delaisMinArcAttente));
        for (; itArretTo != arretsDeLaStation.end(); itArretTo++) {
            Arret::Ptr arretTo = itArretTo->second;
            unsigned int arretToNumeroLigne = voyages.find(arretTo->getVoyageId())->second.getLigne();

            if (arretFromNumeroLigne != arretToNumeroLigne) {
                auto it = arcsPossiblesVersArretTo.find(arretFromNumeroLigne);
                if (it != arcsPossiblesVersArretTo.end()) {
                    if (it->second->getHeureArrivee() > arretTo->getHeureArrivee()) {
                        it->second = arretTo;
                    }
                } else {
                    arcsPossiblesVersArretTo.insert({arretToNumeroLigne, arretTo});
                }
            }
        }

        for (const auto &arret : arcsPossiblesVersArretTo) {
            arcsAttente.insert({arretFrom, arret.second});
        }
    }
    return arcsAttente;

}

//! \brief Permet de récupérer les arcs de transfert qui sont valide
map<Arret::Ptr, map<string, Arret::Ptr>> getArcsDeTransferts(const multimap<Heure, Arret::Ptr> &arretsStationFrom,
                                                             const multimap<Heure, Arret::Ptr> &arretsStationTo,
                                                             unsigned int minTransferTime,
                                                             const map<string, Voyage> &voyages,
                                                             const unordered_map<unsigned int, Ligne> &lignes) {
    map<Arret::Ptr, map<string, Arret::Ptr>> arcsPossibles;

    //On boucle sur tous les arrêts de la station from_station_id
    for (const auto &itArretsFrom : arretsStationFrom) {
        map<string, Arret::Ptr> arretsToPossibles;

        Arret::Ptr arretFrom = itArretsFrom.second;
        auto itArretsTo = arretsStationTo.lower_bound(
                arretFrom->getHeureArrivee().add_secondes(minTransferTime));

        // On boucl sur tous les arrês de la station to_station_id
        for (; itArretsTo != arretsStationTo.end(); itArretsTo++) {
            Arret::Ptr arretTo = itArretsTo->second;

            const string &fromLigneNumero = lignes.find(
                    voyages.at(arretFrom->getVoyageId()).getLigne())->second.getNumero();
            const string &toLigneNumero = lignes.find(
                    voyages.at(arretTo->getVoyageId()).getLigne())->second.getNumero();

            if (fromLigneNumero != toLigneNumero) {
                auto it = arretsToPossibles.find(toLigneNumero);
                if (it != arretsToPossibles.end()) {
                    if (it->second->getHeureArrivee() > arretTo->getHeureArrivee()) {
                        it->second = arretFrom;
                    }
                } else {
                    arretsToPossibles.insert({toLigneNumero, arretFrom});
                }
            }
        }
        arcsPossibles.insert({arretFrom, arretsToPossibles});
    }
    return arcsPossibles;
}

//! \brief ajout des arcs dus aux voyages
//! \brief insère les arrêts (associés aux sommets) dans m_arretDuSommet et
//! m_sommetDeArret \throws logic_error si une incohérence est détecté lors de
//! cette étape de construction du graphe
void ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS &p_gtfs) {
    try {
        size_t idArret = 0;
        // Boucle sur tous les voyages de getVoyages() de l'objet p_gtfs
        for (const auto &itVoyages : p_gtfs.getVoyages()) {
            // On boucle sur tous les arrets de getArrets() de l'objet voyage
            const auto &arrets = itVoyages.second.getArrets();

            for (const auto &arret : arrets) {
                m_arretDuSommet.push_back(arret);
                m_sommetDeArret.insert({arret, idArret});

                if (arret != *arrets.begin()) {
                    unsigned int poids = arret->getHeureArrivee() - m_arretDuSommet[idArret - 1]->getHeureArrivee();
                    m_leGraphe.ajouterArc(idArret - 1, idArret, poids);
                }

                idArret++;
            }
        }
        if (idArret == 0) {
            throw logic_error("Aucun arrêts a été ajouté.");
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}


//! \brief ajouts des arcs dus aux transferts entre stations
//! \throws logic_error si une incohérence est détecté lors de cette étape de
//! construction du graphe
void ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS &p_gtfs) {
    try {
        const vector<tuple<unsigned int, unsigned int, unsigned int>> &transferts = p_gtfs.getTransferts();
        const map<unsigned int, Station> &stations = p_gtfs.getStations();
        const map<string, Voyage> &voyages = p_gtfs.getVoyages();
        const unordered_map<unsigned int, Ligne> &lignes = p_gtfs.getLignes();

        // On boucle sur tous les transferts
        for (const auto &transfert : transferts) {
            const unsigned int fromStationId = get<0>(transfert);
            const unsigned int toStationId = get<1>(transfert);
            unsigned int minTransferTime = get<2>(transfert);

            const multimap<Heure, Arret::Ptr> &arretsStationFrom = stations.at(fromStationId).getArrets();
            const multimap<Heure, Arret::Ptr> &arretsStationTo = stations.at(toStationId).getArrets();

            const map<Arret::Ptr, map<string, Arret::Ptr>> arcs = getArcsDeTransferts(
                    arretsStationFrom, arretsStationTo, minTransferTime, voyages, lignes
            );

            for (const auto &arretFrom: arcs) {
                for (const auto &arretTo : arretFrom.second) {
                    unsigned int poids = arretTo.second->getHeureArrivee() - arretFrom.first->getHeureArrivee();
                    size_t idArretFrom = m_sommetDeArret[arretFrom.first];
                    size_t idArretTo = m_sommetDeArret[arretTo.second];

                    m_leGraphe.ajouterArc(idArretFrom, idArretTo, poids);
                }
            }
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajouts des arcs d'une station à elle-même pour les stations qui ne
//! sont pas dans DonneesGTFS::m_stationsDeTransfert \throws logic_error si une
//! incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsAttente(const DonneesGTFS &p_gtfs) {
    try {
        const map<unsigned int, Station> &stations = p_gtfs.getStations();
        const vector<tuple<unsigned int, unsigned int, unsigned int>> &transferts = p_gtfs.getTransferts();
        const map<string, Voyage> &voyages = p_gtfs.getVoyages();

        for (const auto &itStations : stations) {
            const Station &station = itStations.second;

            if (!isStationPresenteDansTransfert(station, transferts)) {
                const multimap<Heure, Arret::Ptr> &arretsDeLaStation = station.getArrets();

                const multimap<Arret::Ptr, Arret::Ptr> arcsAttentes =
                        getArcsAttente(delaisMinArcsAttente, station, arretsDeLaStation, voyages);

                for (const auto &pair: arcsAttentes) {
                    unsigned int poids = pair.second->getHeureArrivee() - pair.first->getHeureArrivee();
                    unsigned int idDepart = m_sommetDeArret[pair.first];
                    unsigned int idArrive = m_sommetDeArret[pair.second];

                    m_leGraphe.ajouterArc(idDepart, idArrive, poids);
                }
            }
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajoute des arcs au réseau GTFS à partir des données GTFS
//! \brief Il s'agit des arcs allant du point origine vers une station si
//! celle-ci est accessible à pieds et des arcs allant d'une station vers le
//! point destination \param[in] p_gtfs: un objet DonneesGTFS \param[in]
//! p_pointOrigine: les coordonnées GPS du point origine \param[in]
//! p_pointDestination: les coordonnées GPS du point destination \throws
//! logic_error si une incohérence est détecté lors de la construction du graphe
//! \post constuit un réseau GTFS représenté par un graphe orienté pondéré avec
//! poids non négatifs \post assigne la variable m_origine_dest_ajoute à true
//! (car les points orignine et destination font parti du graphe) \post insère
//! dans m_sommetsVersDestination les numéros de sommets connctés au point
//! destination
void ReseauGTFS::ajouterArcsOrigineDestination(const DonneesGTFS &p_gtfs, const Coordonnees &p_pointOrigine,
                                               const Coordonnees &p_pointDestination) {
    try {
        tuple<Arret::Ptr, Arret::Ptr> arretsOrigineDestination =
                creerArretsOrigineDestination(stationIdOrigine, stationIdDestination, getNbArcs());
        Arret::Ptr arretOrigine = get<0>(arretsOrigineDestination);
        Arret::Ptr arretDestination = get<1>(arretsOrigineDestination);

        m_sommetOrigine = m_sommetDeArret.size(); // Prochain numéro de sommet disponible
        m_sommetDeArret.insert({arretOrigine, m_sommetOrigine});
        m_arretDuSommet.push_back(arretOrigine);

        m_sommetDestination = m_sommetDeArret.size();
        m_sommetDeArret.insert({arretDestination, m_sommetDestination});
        m_arretDuSommet.push_back(arretDestination);

        m_leGraphe.resize(m_leGraphe.getNbSommets() + 2);


        multimap<Arret::Ptr, Coordonnees> arretsAtteignablesDepuisOrigine = getArretsAtteingnableAPiedDepuisOrigine(
                arretOrigine, p_pointOrigine, distanceMaxMarche, p_gtfs.getStations(), p_gtfs.getVoyages());
        m_nbArcsOrigineVersStations = 0;
        for (const auto &pair: arretsAtteignablesDepuisOrigine) {
            size_t idArcOrigine = m_sommetOrigine;
            size_t idArcDestination = m_sommetDeArret[pair.first];
            Coordonnees coordonneesArret = pair.second;
            unsigned int poids = getPoidsEntre2Coord(vitesseDeMarche, distanceMaxMarche, p_pointOrigine,
                                                     coordonneesArret);

            m_leGraphe.ajouterArc(idArcOrigine, idArcDestination, poids);
            m_nbArcsOrigineVersStations++;
        }

        multimap<Arret::Ptr, Coordonnees> arretsAtteignableDepuisDestination = getArretsEntreStationsEtDestination(
                arretDestination, p_pointDestination, distanceMaxMarche, p_gtfs.getStations());
        for (const auto &pair : arretsAtteignableDepuisDestination) {
            size_t idArcOrigine = m_sommetDeArret[pair.first];
            size_t idArcDestination = m_sommetDestination;
            Coordonnees coordonneesArret = pair.second;
            unsigned int poids = getPoidsEntre2Coord(vitesseDeMarche, distanceMaxMarche, p_pointDestination,
                                                     coordonneesArret);

            m_leGraphe.ajouterArc(idArcOrigine, idArcDestination, poids);
            m_sommetsVersDestination.push_back(idArcOrigine);
        }
        m_nbArcsStationsVersDestination = m_sommetsVersDestination.size();

        m_origine_dest_ajoute = true;
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief Remet ReseauGTFS dans l'était qu'il était avant l'exécution de
//! ReseauGTFS::ajouterArcsOrigineDestination() \param[in] p_gtfs: un objet
//! DonneesGTFS \throws logic_error si une incohérence est détecté lors de la
//! modification du graphe \post Enlève de ReaseauGTFS tous les arcs allant du
//! point source vers un arrêt de station et ceux allant d'un arrêt de station
//! vers la destination \post assigne la variable m_origine_dest_ajoute à false
//! (les points orignine et destination sont enlevés du graphe) \post enlève les
//! données de m_sommetsVersDestination
void ReseauGTFS::enleverArcsOrigineDestination() {
    try {
        for (size_t i : m_sommetsVersDestination) {
            m_leGraphe.enleverArc(i, m_sommetDestination);
        }

        m_leGraphe.resize(m_leGraphe.getNbSommets() - 2);

        Arret::Ptr arretOrigine = m_arretDuSommet[m_sommetOrigine];
        Arret::Ptr arretDestination = m_arretDuSommet[m_sommetDestination];
        m_sommetDeArret.erase(arretOrigine);
        m_sommetDeArret.erase(arretDestination);
        m_arretDuSommet.resize(m_arretDuSommet.size() - 2);

        m_nbArcsStationsVersDestination = 0;
        m_nbArcsOrigineVersStations = 0;
        m_origine_dest_ajoute = false;

    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}
