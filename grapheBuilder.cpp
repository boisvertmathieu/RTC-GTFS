//
// Created by Mario Marchand on 2016-12-29.
// Updated by Mathieu Boisvert on 2021-12-07
//

#include "DonneesGTFS.h"
#include <fstream>
#include <algorithm>

using namespace std;

void supprimerStationsSansArrets(map<unsigned int, Station> &stations) {
    for (auto it = stations.cbegin(); it != stations.cend();) {
        if (it->second.getArrets().empty()) {
            it = stations.erase(it);
        } else {
            it++;
        }
    }
}

void supprimerVoyageSansArrets(map<string, Voyage> &voyages) {
    for (auto it = voyages.cbegin(); it != voyages.cend();) {
        if (it->second.getArrets().empty()) {
            it = voyages.erase(it);
        } else {
            it++;
        }
    }
}

void supprimerGuillemetsDansString(string &string) {
    const char unwanted_char = '"';
    string.erase(remove(string.begin(), string.end(), unwanted_char), string.end());
}

Date *construireDateDepuisString(const string &str_date) {
    int an = stoi(str_date.substr(0, 4));
    int mois = stoi(str_date.substr(4, 2));
    int jour = stoi(str_date.substr(6, 2));

    return new Date(an, mois, jour);
}

Heure *construireHeureDepuisString(const string &str_heure) {

    const char delimiter = ':';
    size_t curr_pos = 0;
    size_t next_pos = 0;
    vector<unsigned int> values(3);
    int index = 0;

    while (next_pos != string::npos && index < values.size()) {
        next_pos = str_heure.find(delimiter, curr_pos);

        string curr_value = str_heure.substr(curr_pos, next_pos);

        values[index] = stoi(curr_value);

        curr_pos = next_pos + 1;
        index++;
    }

    return new Heure(values[0], values[1], values[2]);
}


//! \brief ajoute les lignes dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les lignes
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterLignes(const std::string &p_nomFichier) {
    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            supprimerGuillemetsDansString(ligne);
            vector<string> vector = string_to_vector(ligne, ',');

            unsigned int route_id = stoi(vector[0]);
            const string route_short_name = vector[2];
            const string description = vector[4];

            CategorieBus categorie = Ligne::couleurToCategorie(vector[7]);
            Ligne l(route_id, route_short_name, description, categorie);

            m_lignes.insert(make_pair(route_id, l));
            m_lignes_par_numero.insert(make_pair(route_short_name, l));
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajoute les stations dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterStations(const std::string &p_nomFichier) {

    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            supprimerGuillemetsDansString(ligne);
            vector<string> vector = string_to_vector(ligne, ',');

            unsigned int stop_id = stoi(vector[0]);
            const string station_nom = vector[1];
            const string station_description = vector[2];

            Coordonnees coordonnees(stod(vector[3]), stod(vector[4]));
            Station station(stop_id, station_nom, station_description, coordonnees);

            m_stations.insert(make_pair(stop_id, station));
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajoute les transferts dans l'objet GTFS
//! \breif Cette méthode doit âtre utilisée uniquement après que tous les arrêts ont été ajoutés
//! \brief les transferts (entre stations) ajoutés sont uniquement ceux pour lesquelles les stations sont prensentes dans l'objet GTFS
//! \brief les transferts sont ajoutés dans m_transferts
//! \brief les from_station_id des stations de transfert sont ajoutés dans m_stationsDeTransfert
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
//! \throws logic_error si tous les arrets de la date et de l'intervalle n'ont pas été ajoutés
void DonneesGTFS::ajouterTransferts(const std::string &p_nomFichier) {

    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            vector<string> vector = string_to_vector(ligne, ',');

            unsigned int from_station_id = stoi(vector[0]);
            unsigned int to_station_id = stoi(vector[1]);
            unsigned int min_transfer_time = stoi(vector[3]);

            if (min_transfer_time == 0) {
                min_transfer_time = 1;
            }

            if (m_stations.find(from_station_id) != m_stations.end() &&
                m_stations.find(to_station_id) != m_stations.end()) {
                m_transferts.emplace_back(from_station_id, to_station_id, min_transfer_time);
                m_stationsDeTransfert.insert(from_station_id);
            }
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajoute les services de la date du GTFS (m_date)
//! \param[in] p_nomFichier: le nom du fichier contenant les services
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterServices(const std::string &p_nomFichier) {

    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            vector<string> vector = string_to_vector(ligne, ',');

            const string service_id = vector[0];
            const string str_date = vector[1];
            const string exception_type = vector[2];

            if (exception_type == "1") {
                Date *date = construireDateDepuisString(str_date);

                if (m_date == *date) {
                    m_services.insert(service_id);
                }
                delete date;
            }
        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}

//! \brief ajoute les voyages de la date
//! \brief seuls les voyages dont le service est présent dans l'objet GTFS sont ajoutés
//! \param[in] p_nomFichier: le nom du fichier contenant les voyages
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterVoyagesDeLaDate(const std::string &p_nomFichier) {

    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            supprimerGuillemetsDansString(ligne);
            vector<string> vector = string_to_vector(ligne, ',');

            const string service_id = vector[1];

            if (m_services.find(service_id) != m_services.end()) {
                unsigned int route_id = stoi(vector[0]);
                const string trip_id = vector[2];
                const string trip_headsign = vector[3];

                Voyage voyage(trip_id, route_id, service_id, trip_headsign);
                m_voyages.insert(make_pair(trip_id, voyage));
            }

        }
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }

}

//! \brief ajoute les arrets aux voyages présents dans le GTFS si l'heure du voyage appartient à l'intervalle de temps du GTFS
//! \brief Un arrêt est ajouté SSI son heure de départ est >= now1 et que son heure d'arrivée est < now2
//! \brief De plus, on enlève les voyages qui n'ont pas d'arrêts dans l'intervalle de temps du GTFS
//! \brief De plus, on enlève les stations qui n'ont pas d'arrets dans l'intervalle de temps du GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les arrets
//! \post assigne m_tousLesArretsPresents à true
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterArretsDesVoyagesDeLaDate(const std::string &p_nomFichier) {
    ifstream fichier(p_nomFichier);
    string ligne;

    try {
        getline(fichier, ligne);

        while (getline(fichier, ligne)) {
            vector<string> vector = string_to_vector(ligne, ',');

            Heure *heure_arrivee = construireHeureDepuisString(vector[1]);
            Heure *heure_depart = construireHeureDepuisString(vector[2]);

            if (*heure_depart >= m_now1 && *heure_arrivee < m_now2) {

                const string voyage_id = vector[0];
                if (m_voyages.find(voyage_id) != m_voyages.end()) {

                    unsigned int station_id = stoi(vector[3]);
                    unsigned int numero_sequence = stoi(vector[4]);

                    Arret::Ptr arret = make_shared<Arret>(station_id, *heure_arrivee, *heure_depart, numero_sequence,
                                                          voyage_id);

                    m_voyages.at(voyage_id).ajouterArret(arret);
                    m_stations.at(station_id).addArret(arret);
                    m_nbArrets++;
                }
            }

            delete heure_arrivee;
            delete heure_depart;
        }

        supprimerVoyageSansArrets(m_voyages);
        supprimerStationsSansArrets(m_stations);

        m_tousLesArretsPresents = true;
    } catch (exception &ex) {
        throw logic_error(ex.what());
    }
}



