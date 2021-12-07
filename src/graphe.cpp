//
//  Graphe.cpp
//  Classe pour graphes orientés pondérés (non négativement) avec listes d'adjacence
//
//  Mario Marchand automne 2016.
//

#include "graphe.h"
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

using namespace std;

//! \brief Constructeur avec paramètre du nombre de sommets désiré
//! \param[in] p_nbSommets indique le nombre de sommets désiré
//! \post crée le vecteur de p_nbSommets de listes d'adjacence vides avec nbArcs=0
Graphe::Graphe(size_t p_nbSommets) : m_listesAdj(p_nbSommets), m_nbArcs(0) {}

//! \brief change le nombre de sommets du graphe
//! \param[in] p_nouvelleTaille indique le nouveau nombre de sommet
//! \post le graphe est un vecteur de p_nouvelleTaille de listes d'adjacence
//! \post les anciennes listes d'adjacence sont toujours présentes lorsque p_nouvelleTaille >= à l'ancienne taille
//! \post les dernières listes d'adjacence sont enlevées lorsque p_nouvelleTaille < à l'ancienne taille
//! \post nbArcs est diminué par le nombre d'arcs sortant des sommets à enlever si certaines listes d'adgacence sont
//! supprimées
void Graphe::resize(size_t p_nouvelleTaille) {
    if (p_nouvelleTaille < m_listesAdj.size()) // certaines listes d'adj seront supprimées
    {
        // diminuer nbArcs par le nb d'arcs sortant des sommets à enlever
        for (size_t i = p_nouvelleTaille; i < m_listesAdj.size(); ++i) {
            m_nbArcs -= m_listesAdj[i].size();
        }
    }
    m_listesAdj.resize(p_nouvelleTaille);
}

size_t Graphe::getNbSommets() const { return m_listesAdj.size(); }

size_t Graphe::getNbArcs() const { return m_nbArcs; }

//! \brief ajoute un arc d'un poids donné dans le graphe
//! \param[in] i: le sommet origine de l'arc
//! \param[in] j: le sommet destination de l'arc
//! \param[in] poids: le poids de l'arc
//! \pre les sommets i et j doivent exister
//! \throws logic_error lorsque le sommet i ou le sommet j n'existe pas
//! \throws logic_error lorsque le poids == numeric_limits<unsigned int>::max()
void Graphe::ajouterArc(size_t i, size_t j, unsigned int poids) {
    if (i >= m_listesAdj.size())
        throw logic_error("Graphe::ajouterArc(): tentative d'ajouter l'arc(i,j) avec un sommet i inexistant");
    if (j >= m_listesAdj.size())
        throw logic_error("Graphe::ajouterArc(): tentative d'ajouter l'arc(i,j) avec un sommet j inexistant");
    if (poids == numeric_limits<unsigned int>::max())
        throw logic_error("Graphe::ajouterArc(): valeur de poids interdite");
    m_listesAdj[i].emplace_back(Arc(j, poids));
    ++m_nbArcs;
}

//! \brief enlève un arc dans le graphe
//! \param[in] i: le sommet origine de l'arc
//! \param[in] j: le sommet destination de l'arc
//! \pre l'arc (i,j) et les sommets i et j dovent exister
//! \post enlève l'arc mais n'enlève jamais le sommet i
//! \throws logic_error lorsque le sommet i ou le sommet j n'existe pas
//! \throws logic_error lorsque l'arc n'existe pas
void Graphe::enleverArc(size_t i, size_t j) {
    if (i >= m_listesAdj.size())
        throw logic_error("Graphe::enleverArc(): tentative d'enlever l'arc(i,j) avec un sommet i inexistant");
    if (j >= m_listesAdj.size())
        throw logic_error("Graphe::enleverArc(): tentative d'enlever l'arc(i,j) avec un sommet j inexistant");
    auto &liste = m_listesAdj[i];
    bool arc_enleve = false;
    if (liste.empty())
        throw logic_error("Graphe:enleverArc(): m_listesAdj[i] est vide");
    for (auto itr = liste.end(); itr != liste.begin();) // on débute par la fin par choix
    {
        if ((--itr)->destination == j) {
            liste.erase(itr);
            arc_enleve = true;
            break;
        }
    }
    if (!arc_enleve)
        throw logic_error("Graphe::enleverArc: cet arc n'existe pas; donc impossible de l'enlever");
    --m_nbArcs;
}

unsigned int Graphe::getPoids(size_t i, size_t j) const {
    if (i >= m_listesAdj.size())
        throw logic_error("Graphe::getPoids(): l'incice i n,est pas un sommet existant");
    for (auto &arc : m_listesAdj[i]) {
        if (arc.destination == j)
            return arc.poids;
    }
    throw logic_error("Graphe::getPoids(): l'arc(i,j) est inexistant");
}

//! \brief Permet de trouver le plus court chemin entre le sommet p_origine et le sommet p_destination
//! \param[in] p_origine: le sommet de départ \param[in] p_destination: le sommet de destination \param[out]
//! p_chemin: le plus court chemin trouvé entre p_origine et p_destination \return Le poids du chemin le plus court
//! entre p_origine et p_destination \throws logic_error lorsque p_origine ou p_destination n'existe pas
unsigned int Graphe::plusCourtChemin(size_t p_origine, size_t p_destination, vector<size_t> &p_chemin) const {
    p_chemin.clear();

    if (p_origine >= m_listesAdj.size() || p_destination >= m_listesAdj.size()) {
        throw logic_error("Graphe::dijkstra(): p_origine ou p_destination n'existe pas");
    }
    if (p_origine == p_destination) {
        p_chemin.push_back(p_destination);
        return 0;
    }

    size_t tailleGraphe = m_listesAdj.size();
    vector<size_t> poids(tailleGraphe, numeric_limits<size_t>::max());
    vector<bool> sommetsSolutionnes(tailleGraphe, false);
    vector<size_t> predecesseur(tailleGraphe, numeric_limits<size_t>::max());

    typedef size_t DistanceSommet;
    typedef size_t NumeroSommet;
    typedef pair<DistanceSommet, NumeroSommet> sommet;
    // Contient les sommets solutionnés en bordures des sommets non solutionnés
    priority_queue<sommet, vector<sommet>, greater<sommet>> q;

    poids[p_origine] = 0;
    q.push({0, p_origine});

    while (!q.empty()) {
        size_t numeroSommetCourant = q.top().second;
        if (numeroSommetCourant == p_destination) {
            // Le sommet est le sommet destination, on a trouvé le chemin le plus court
            break;
        }

        // À la prochaine itération, le sommet courant n'aura plus de sommet adjacent non solutionnés, on l'enlève donc
        // de q ici
        q.pop();
        sommetsSolutionnes[numeroSommetCourant] = true;

        // On boucle sur tous les arc (sommets adjacent) du sommet courant
        auto arc = m_listesAdj[numeroSommetCourant].begin();
        for (; arc != m_listesAdj[numeroSommetCourant].end(); ++arc) {
            // Si le sommet de l'arc n'est pas solutionné
            if (!sommetsSolutionnes[arc->destination]) {
                // On récupère la distance entre le sommet courrant et le sommet de l'arc
                unsigned int distanceSommetAdjacent = poids[numeroSommetCourant] + arc->poids;
                // On récupère le numero du sommet de l'arc
                size_t numeroSommetAdjacent = arc->destination;
                // Si la distance entre le sommet courant et le sommet de l'arc est plus petit que le poids enregistré
                // au numéro correspondant au sommet de l'arc
                if (distanceSommetAdjacent < poids[numeroSommetAdjacent]) {
                    // On remplace le poids enregistré à ce numero de sommet
                    poids[arc->destination] = distanceSommetAdjacent;

                    // On ajoute à la sommet adjacent à "q" afin de solutionner ses sommets adjacent s'il y en a
                    q.push({poids[numeroSommetAdjacent], numeroSommetAdjacent});
                    predecesseur[arc->destination] = numeroSommetCourant;
                }
            }
        }
    }

    // Impossible de se rendre à p_destination depuis p_origine
    if (predecesseur[p_destination] == numeric_limits<unsigned int>::max()) {
        p_chemin.push_back(p_destination);
        return numeric_limits<unsigned int>::max();
    }

    // Solution possible, on créer le chemin dans p_chemin à l'aide des predecesseurs
    stack<NumeroSommet> chemin;
    chemin.push(p_destination);

    NumeroSommet numero = p_destination;
    while (predecesseur[numero] != numeric_limits<size_t>::max()) {
        numero = predecesseur[numero];
        chemin.push(numero);
    }

    while (!chemin.empty()) {
        size_t temp = chemin.top();
        p_chemin.push_back(temp);
        chemin.pop();
    }

    return poids[p_destination];
}
