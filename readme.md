compiler cpp:

cmake -B build -DSFML_ROOT=/chemin/vers/sfml
cmake --build build --config Release

lancer jeu cpp (depuis snake/):

./cpp/build/Release/snake_math.exe

# Installer SFML (system, pas besoin de SFML_ROOT)
sudo apt install libsfml-dev

cd snake/cpp
cmake -B build
cmake --build build
./build/snake_math        # assets copiés automatiquement à côté de l'exe

cmake -B build -DSFML_ROOT=~/sfml-2.6.1
