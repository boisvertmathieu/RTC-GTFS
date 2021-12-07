//
// Produced by Mario on Dec 2016.
// Update by Mathieu Boisver on 2021-12-07
//

#include <iostream>

#include "DonneesGTFS.h"

using namespace std;

int main()
{

    const std::string chemin_dossier = "data";

    Date today(2020, 9, 25);
    Heure now1(7, 30, 0);
//    Date today; //Le constructeur par défaut initialise la date à aujourd'hui
//    Heure now1; //Le constructeur par défaut initialise l'heure à maintenant
    Heure now2 = now1.add_secondes(3600);

    DonneesGTFS donnees_rtc(today, now1, now2);

    donnees_rtc.ajouterLignes(chemin_dossier + "/routes.txt");
    cout << "Nombre de lignes = " << donnees_rtc.getNbLignes() << endl;
    donnees_rtc.ajouterStations(chemin_dossier + "/stops.txt");
    cout << "Nombre de stations initiales = " << donnees_rtc.getNbStations() << endl;
    donnees_rtc.ajouterServices(chemin_dossier + "/calendar_dates.txt");
    cout << "Nombre de services = " << donnees_rtc.getNbServices() << endl;
    donnees_rtc.ajouterVoyagesDeLaDate(chemin_dossier + "/trips.txt");
    donnees_rtc.ajouterArretsDesVoyagesDeLaDate(chemin_dossier + "/stop_times.txt");
    cout << "Nombre de stations ayant au moins 1 arret = " << donnees_rtc.getNbStations() << endl;
    donnees_rtc.ajouterTransferts(chemin_dossier + "/transfers.txt");
    cout << "Nombre de transferts = " << donnees_rtc.getNbTransferts() << endl;
    cout << "Nombres de voyages = " << donnees_rtc.getNbVoyages() << endl;
    cout << "Nombre d'arrets = " << donnees_rtc.getNbArrets() << endl << endl;

    donnees_rtc.afficherLignes();
    donnees_rtc.afficherStations();
    donnees_rtc.afficherStationsDeTransfert();
    donnees_rtc.afficherTransferts();
    donnees_rtc.afficherArretsParVoyages();
    donnees_rtc.afficherArretsParStations();

    return 0;
}
