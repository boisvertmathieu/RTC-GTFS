# RTC-GTFS
Projet en C++ visant à implémenter un algorithme de tri permettant de déterminer le chemin le plus court entre 2 arrêt du Réseau de transport de la Capitale (RTC)
### Installation
Vous devez préalablement avoir installé CMake et un compilateur C++ tel que [gcc](https://gcc.gnu.org/) sur votre machine. La version minmale requise de CMake 3.4 et votre compilateur doit pouvoir supporter C++ 11 (on ajoute le flag `-std=c++11` dans le `CMakeList.txt`).

Après avoir cloner le projet. Vous devez vous créer une dossier `build` à la racine du projet : 
```
git clone git@github.com:boisvertmathieu/RTC-GTFS.git
cd RTC-GTFS
mkdir build
```
