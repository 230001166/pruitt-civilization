#include "Game.h"
#include <iostream>
#include <random>
#include <time.h>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>

#include "Unit.h"
#include "Trade.h"
#include "Resource.h"
#include "MapGenerator.h"
#include "textRenderer.h"
#include "sharedMethods.h"
#include "GameUpdater.h"
#include "AI.h"
#include "LoanEvent.h"
#include "AllianceEvent.h"

using namespace std;

Game::Game()
{
    //ctor
}

Game::~Game()
{
    //dtor
}

textRenderer renderer;
AI ai;

GameVariables gameVariables;

const std::string resourceNames[25] = {"None","Horses","Iron","Rubber","Copper","Pearls","Crab","Whales","Salt","Spices","Stone","Marble","Cocoa","Sheep","Cattle","Diamonds","Nutmeg","Ginger","Silk","Dyes","Citrus","Ivory","Furs","Silver","Gold"};

bool loopVariable = true;

    const int cityExpansionLimit = 3;

int turnNumber = 0;
bool gameLoopVariable = true;

enum characterActionCodes : char {
    EXPLORATION = 'x',
    UNITS = 'u',
    MOVE = 'm',
    BUY = 'b',
    SPECIAL = 's',
    END = 'e',
};

enum focuses : int {
    POPULATION,
    PRODUCTION,
    GOLD,
    SCIENCE
};

int Game::returnTerritoryTiles (int x, int y, int Civ_index) {

    int TerritoryTiles = 0;

    if (x > 0){
        if (gameVariables.worldMap.WorldTerritoryMap[x-1][y] == (Civ_index+1)){
            TerritoryTiles++;
        }
    }

    if (y > 0){
        if (gameVariables.worldMap.WorldTerritoryMap[x][y-1] == (Civ_index+1)){
            TerritoryTiles++;
        }
    }

    if (y < (gameVariables.worldMap.worldSize*4-1)){
        if (gameVariables.worldMap.WorldTerritoryMap[x][y+1] == (Civ_index+1)){
            TerritoryTiles++;
        }
    }

    if (x < (gameVariables.worldMap.worldSize-1)){
        if (gameVariables.worldMap.WorldTerritoryMap[x+1][y] == (Civ_index+1)){
            TerritoryTiles++;
        }
    }

    return TerritoryTiles;
}

bool Game::isLandTile (int x, int y) {

    if (gameVariables.worldMap.featureMap[x][y] != gameVariables.worldMap.mapTiles::COAST
        && gameVariables.worldMap.featureMap[x][y] != gameVariables.worldMap.mapTiles::OCEAN) {

        return true;

    } else {

        return false;

    }

}

int Game::returnLandTiles (int x, int y){

    int LandTiles = 0;

    if (x > 0) {

        if (isLandTile(x-1,y)) {

            LandTiles++;

        }

    }

    if (y > 0) {

        if (isLandTile(x,y-1)) {

            LandTiles++;

        }

    }

    if (x < gameVariables.worldMap.worldSize-1) {

        if (isLandTile(x+1,y)) {

            LandTiles++;

        }

    }

    if (y < gameVariables.worldMap.worldSize*4-1) {

        if (isLandTile(x+1,y)) {

            LandTiles++;

        }

    }

    if (x > 0 && y > 0) {

        if (isLandTile(x-1,y-1)) {

            LandTiles++;

        }

    }

    if (x > 0 && y < (gameVariables.worldMap.worldSize*4-1)) {

        if (isLandTile(x-1,y+1)) {

            LandTiles++;

        }

    }

    if (x < (gameVariables.worldMap.worldSize-1) && y > 0) {

        if (isLandTile(x+1,y-1)) {

            LandTiles++;

        }

    }

    if (x < (gameVariables.worldMap.worldSize-1) && y < (gameVariables.worldMap.worldSize*4-1)) {

        if (isLandTile(x+1,y+1)) {

            LandTiles++;

        }

    }

    return LandTiles;
}

void Game::setupWorld () {

    loadUnitsFromFile("units.sav");
    loadBuildingsFromFile("buildings.sav");

    MapGenerator mapGen;
    std::cout << "What type of world do you want to be generated?\n[1] Continents\n[2] Pangaea" << std::endl;

    int choice = 1; int AmountOfCivilizations = 5;

    choice = sharedMethods::bindIntegerInputToRange(1,2,1);

    switch (choice) {

        case 1:
            mapGen.generateWorld_Continents(gameVariables.worldMap);
        break;
        case 2:
            mapGen.generateWorld_Pangaea(gameVariables.worldMap);
        break;
        default:
            mapGen.generateWorld_Continents(gameVariables.worldMap);
        break;

    }

    std::cout << "How many Civilizations do you want? The maximum is 10." << std::endl;

    AmountOfCivilizations = sharedMethods::bindIntegerInputToRange(2,10,4);

    for (int i = 0; i < AmountOfCivilizations; i++) {

        generateCiv(i);

    }

    std::cout << "How many minor civilizations do you want? The maximum is 8." << std::endl;

    AmountOfCivilizations = sharedMethods::bindIntegerInputToRange(0,8,2);

    for (int i = 0; i < AmountOfCivilizations; i++) {

        generateMinorCiv(i);

    }

    for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

        for (unsigned int j = 0; j < gameVariables.Civilizations.size(); j++) {

            gameVariables.Civilizations[i].hasMetCivilizations.push_back (false);

        }

    }

    setupDiplomacy();

}

void Game::setupDiplomacy () {

    for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

        for (unsigned int j = 0; j < gameVariables.Civilizations.size(); j++) {

            int variantStartDiplomacy = rand() % 50 - 25;
            gameVariables.Civilizations[i].relationsWithOtherCivilizations.push_back(variantStartDiplomacy);

        }

    }

}

void Game::loadTechnologiesFromFile (std::string filename, int civilizationIndex) {

    std::string line;
    ifstream file (filename);

    if (file.is_open())  {

    int techSize = 0, prereqSize = 0, numberOfUnlockableUnits = 0; Research temp;

    file >> techSize;

        for (int i = 0; i < techSize; i++) {

           temp.unlockableUnits.clear();
           temp.prerequisiteTechnologiesRequired.clear();

           file >> temp.researchName;
           file >> temp.type;
           file >> temp.era;
           file >> temp.scienceCostToLearnResearch;
           file >> prereqSize;

           for (int j = 0; j < prereqSize; j++) {

                std::string x;
                file >> x;
                temp.prerequisiteTechnologiesRequired.push_back(x);

           }

           file >> temp.aiFocus_offense;
           file >> temp.aiFocus_defense;
           file >> temp.aiFocus_economic;
           file >> temp.aiFocus_population;
           file >> temp.aiFocus_production;
           file >> temp.aiFocus_diplomatic;
           file >> temp.aiFocus_scientific;
           file >> temp.aiFocus_exploration;
           file >> temp.aiFocus_religion;
           file >> temp.aiFocus_overall_importance;

           file >> numberOfUnlockableUnits;

           for (int j = 0; j < numberOfUnlockableUnits; j++) {

                std::string x;
                file >> x;
                temp.unlockableUnits.push_back(x);

           }

           file >> temp.unlockablePromotion;

           gameVariables.Civilizations[civilizationIndex].technologiesToResearch.push_back(temp);

    }

    file.close();

    } else cout << "Unable to open file";

}

void Game::loadUnitsFromFile (std::string filename) {

    std::string line;
    ifstream file (filename);

    if (file.is_open())  {

    int unitSize;
    Unit temp;

    file >> unitSize;

    for (int i = 0; i < unitSize; i++) {

           file >> temp.name;
           file >> temp.health; temp.maxHealth = temp.health;
           file >> temp.combatStrength;
           file >> temp.maxMovePoints;
           file >> temp.terrainMoveCost;
           file >> temp.productionCost;
           file >> temp.goldCost;

           file >> line; /// just used to skip over lines, need to fix

           file >> temp.aiFocus_defense;
           file >> temp.aiFocus_economic;
           file >> temp.aiFocus_exploration;
           file >> temp.aiFocus_offense;
           file >> temp.aiFocus_overall_importance;
           file >> temp.isRanged;

           if (temp.isRanged == true) {

              file >> temp.rangedCombat;
              file >> temp.range;

           } else {

              temp.range = 1;

           }

           file >> line;

           file >> temp.domain.name;
           file >> temp.domain.canCoastalEmbark;
           file >> temp.domain.canCrossOceans;
           file >> temp.domain.canMoveOnLandTiles;

           if (temp.domain.name == "Land") {

               file >> line; /// just used to skip over lines, need to fix

               file >> temp.grasslandModifier.attackModifier;
               file >> temp.grasslandModifier.defenseModifier;
               file >> temp.forestModifier.attackModifier;
               file >> temp.forestModifier.defenseModifier;
               file >> temp.mountainModifier.attackModifier;
               file >> temp.mountainModifier.defenseModifier;
               file >> temp.snowModifier.attackModifier;
               file >> temp.snowModifier.defenseModifier;
               file >> temp.desertModifier.attackModifier;
               file >> temp.desertModifier.defenseModifier;

           }


           gameVariables.Units.push_back(temp);

    }

    file.close();

    } else cout << "Unable to open file";

}

void Game::loadBuildingsFromFile (std::string filename) {

    std::string line;
    ifstream file (filename);

    if (file.is_open())  {

    int buildingSize;
    Building temp;

    file >> buildingSize;

    for (int i = 0; i < buildingSize; i++) {

           file >> temp.name;
           file >> temp.FoodYield;
           file >> temp.ProductionYield;
           file >> temp.GoldYield;
           file >> temp.ScienceYield;
           file >> temp.FaithYield;

           file >> temp.aiFocus_defense;
           file >> temp.aiFocus_economic;
           file >> temp.aiFocus_exploration;
           file >> temp.aiFocus_offense;
           file >> temp.aiFocus_overall_importance;
           file >> temp.aiFocus_population;
           file >> temp.aiFocus_production;
           file >> temp.aiFocus_religion;
           file >> temp.aiFocus_scientific;
           file >> temp.GoldCost;
           file >> temp.productionCost;

           gameVariables.Buildings.push_back(temp);

    }

    file.close();

    } else cout << "Unable to open file";

}

void Game::loadCivilizationsFromFile (std::string filename, std::vector<Civilization> &civs) {

    std::string line;
    ifstream file (filename);

    if (file.is_open())  {

    int civsSize;
    Civilization temp;

    file >> civsSize;

    for (int i = 0; i < civsSize; i++) {

           file >> temp.CivName;
           file >> temp.CivilizationAdjective;

           file >> temp.ExpansionRate;
           file >> temp.ScienceRate;

           int freeTechSize = 0;

           file >> freeTechSize;

           for (int j = 0; j < freeTechSize; j++) {

                std::string tempString;
                file >> tempString;

                temp.learnedTechnologies.push_back(tempString);

           }

           file >> temp.aiFocus_defense;
           file >> temp.aiFocus_diplomatic;
           file >> temp.aiFocus_economic;
           file >> temp.aiFocus_exploration;
           file >> temp.aiFocus_offense;
           file >> temp.aiFocus_population;
           file >> temp.aiFocus_production;
           file >> temp.aiFocus_religion;
           file >> temp.aiFocus_scientific;
           file >> temp.ai_fairness;

           int cityNamesSize = 0;

           file >> cityNamesSize;

           for (int j = 0; j < cityNamesSize; j++) {

                std::string tempString;
                file >> tempString;

                temp.cityNames.push_back(tempString);

           }

           for (int j = 0; j < 3; j++) {

                file >> temp.rgbValues[j];

           }

           civs.push_back(temp);

    }

    file.close();

    } else cout << "Unable to open file";

}

void Game::saveGame (std::string filename) {

    std::string line;
    ofstream file (filename);

    if (file.is_open()) {

        /// SAVE WORLD DATA

        file << "turn number" << "\n";

        file << turnNumber << "\n";

        file << "map data" << "\n";

        for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {

            for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {

                file << gameVariables.worldMap.featureMap[i][j] << "\n";

                file << gameVariables.worldMap.WorldResourceMap[i][j].ResourceCode << "\n";

                file << gameVariables.worldMap.WorldResourceMap[i][j].AmountOfResource << "\n";

                file << gameVariables.worldMap.WorldTerritoryMap[i][j] << "\n";


            }

        }

        file << "unit data" << "\n";

        file << gameVariables.UnitsInGame.size() << "\n";

        for (int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

            file << " > unit #" << i << "\n";

            file << gameVariables.UnitsInGame[i].parentCivilizationIndex << "\n";

            file << gameVariables.UnitsInGame[i].parentGroupingIndex << "\n";

            file << gameVariables.UnitsInGame[i].name << "\n";

            file << gameVariables.UnitsInGame[i].position.x << "\n";
            file << gameVariables.UnitsInGame[i].position.y << "\n";

            file << gameVariables.UnitsInGame[i].health << "\n";
            file << gameVariables.UnitsInGame[i].maxHealth << "\n";
            file << gameVariables.UnitsInGame[i].combatStrength << "\n";

            file << gameVariables.UnitsInGame[i].isRanged << "\n";

            if (gameVariables.UnitsInGame[i].isRanged) {

                file << gameVariables.UnitsInGame[i].rangedCombat << "\n";

            }

            file << gameVariables.UnitsInGame[i].grasslandModifier.attackModifier << "\n";
            file << gameVariables.UnitsInGame[i].grasslandModifier.defenseModifier << "\n";
            file << gameVariables.UnitsInGame[i].grasslandModifier.experience << "\n";

            file << gameVariables.UnitsInGame[i].mountainModifier.attackModifier << "\n";
            file << gameVariables.UnitsInGame[i].mountainModifier.defenseModifier << "\n";
            file << gameVariables.UnitsInGame[i].mountainModifier.experience << "\n";

            file << gameVariables.UnitsInGame[i].forestModifier.attackModifier << "\n";
            file << gameVariables.UnitsInGame[i].forestModifier.defenseModifier << "\n";
            file << gameVariables.UnitsInGame[i].forestModifier.experience << "\n";

            file << gameVariables.UnitsInGame[i].desertModifier.attackModifier << "\n";
            file << gameVariables.UnitsInGame[i].desertModifier.defenseModifier << "\n";
            file << gameVariables.UnitsInGame[i].desertModifier.experience << "\n";

            file << gameVariables.UnitsInGame[i].snowModifier.attackModifier << "\n";
            file << gameVariables.UnitsInGame[i].snowModifier.defenseModifier << "\n";
            file << gameVariables.UnitsInGame[i].snowModifier.experience << "\n";

            file << gameVariables.UnitsInGame[i].movementPoints << "\n";
            file << gameVariables.UnitsInGame[i].maxMovePoints << "\n";

            file << gameVariables.UnitsInGame[i].destinationHasBeenAssigned << "\n";

            file << gameVariables.UnitsInGame[i].moveQueue.size() << "\n";

            for (unsigned int j = 0; j < gameVariables.UnitsInGame[j].moveQueue.size(); j++) {

                file << gameVariables.UnitsInGame[i].moveQueue[j].x << "\n";
                file << gameVariables.UnitsInGame[i].moveQueue[j].y << "\n";

            }

            file << gameVariables.UnitsInGame[i].isTraining << "\n";

            file << gameVariables.UnitsInGame[i].domain.name << "\n";

            file << gameVariables.UnitsInGame[i].domain.canCoastalEmbark << "\n";
            file << gameVariables.UnitsInGame[i].domain.canCrossOceans << "\n";


        }

        file << "civ data" << "\n";

        file << gameVariables.Civilizations.size() << "\n";

        for (int i = 0; i < gameVariables.Civilizations.size(); i++) {

            Civilization temp_civ = gameVariables.Civilizations[i];

            file << temp_civ.Gold << "\n";

            file << temp_civ.GoldPerTurn << "\n";

            file << temp_civ.Happiness << "\n";

            file << temp_civ.ExpansionRate << "\n";

            file << temp_civ.ScienceRate << "\n";

            file << temp_civ.playedByHumans << "\n";

            if (!temp_civ.playedByHumans) {

                file << temp_civ.aiFocus_offense << "\n";

                file << temp_civ.aiFocus_defense << "\n";

                file << temp_civ.aiFocus_economic << "\n";

                file << temp_civ.aiFocus_population << "\n";

                file << temp_civ.aiFocus_production << "\n";

                file << temp_civ.aiFocus_diplomatic << "\n";

                file << temp_civ.aiFocus_scientific << "\n";

                file << temp_civ.aiFocus_exploration << "\n";

                file << temp_civ.aiFocus_religion << "\n";

                file << temp_civ.ai_fairness << "\n";

            }

            for (int j = 0; j < 3; j++) {

                file << temp_civ.rgbValues[j] << "\n";

            }

            file << temp_civ.relationsWithOtherCivilizations.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.relationsWithOtherCivilizations.size(); j++) {

                file << temp_civ.relationsWithOtherCivilizations[j] << "\n";

            }

            for (unsigned int j = 0; j < gameVariables.worldMap.worldSize; j++) {

                for (unsigned int k = 0; k < gameVariables.worldMap.worldSize*4; k++) {

                    file << temp_civ.WorldExplorationMap[j][k] << "\n";

                }

            }

            file << temp_civ.researchPoints << "\n";

            file << temp_civ.learnedTechnologies.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.learnedTechnologies.size(); j++) {

                file << temp_civ.learnedTechnologies[j] << "\n";

            }

            file << temp_civ.technologiesToResearch.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.technologiesToResearch.size(); j++) {

                file << temp_civ.technologiesToResearch[j].researchName << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_offense << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_defense << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_economic << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_population << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_production << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_diplomatic << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_scientific << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_exploration << "\n";

                file << temp_civ.technologiesToResearch[j].aiFocus_religion << "\n";

                file << temp_civ.technologiesToResearch[j].scienceCostToLearnResearch << "\n";

                file << temp_civ.technologiesToResearch[j].prerequisiteTechnologiesRequired.size();

                for (unsigned int k = 0; k < temp_civ.technologiesToResearch[j].prerequisiteTechnologiesRequired.size(); k++) {

                    file << temp_civ.technologiesToResearch[j].prerequisiteTechnologiesRequired[k] << "\n";

                }

            }

            file << temp_civ.resources.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.resources.size(); j++) {

                file << temp_civ.resources[j].AmountOfResource << "\n";
                file << temp_civ.resources[j].ResourceCode << "\n";

            }

            file << temp_civ.AvailableUnitsToCreate.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.AvailableUnitsToCreate.size(); j++) {

                file << temp_civ.AvailableUnitsToCreate[j] << "\n";

            }

            file << temp_civ.cityNames.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.cityNames.size(); j++) {

                file << temp_civ.cityNames[j] << "\n";

            }

            file << "unit groupings\n";

            file << temp_civ.unitGroups.size() << "\n";

            for (unsigned int j = 0; j < temp_civ.unitGroups.size(); j++) {

                for (int k = 0; k < 3; k++) {

                    file << temp_civ.unitGroups[j].rgbValue[k] << "\n";

                }

                file << temp_civ.unitGroups[j].name << "\n";

                file << temp_civ.unitGroups[j].memberUnitIndices.size() << "\n";

                for (unsigned int k = 0; k < temp_civ.unitGroups[j].memberUnitIndices.size(); k++) {

                    file << temp_civ.unitGroups[j].memberUnitIndices[k] << "\n";

                }

            }

        }


        file << "city data \n";

        file << gameVariables.Cities.size() << "\n";

        for (unsigned int i = 0; i < gameVariables.Cities.size(); i++) {

            file << gameVariables.Cities[i].cityName << "\n";

            file << gameVariables.Cities[i].isCapital << "\n";

            file << gameVariables.Cities[i].parentIndex << "\n";

            file << gameVariables.Cities[i].turnsToExpand << "\n";

            file << gameVariables.Cities[i].position.x << "\n";
            file << gameVariables.Cities[i].position.y << "\n";

            file << gameVariables.Cities[i].Population << "\n";

            file << gameVariables.Cities[i].Production << "\n";
            file << gameVariables.Cities[i].ProductionPerTurn << "\n";
            file << gameVariables.Cities[i].ProductionFromTiles << "\n";
            file << gameVariables.Cities[i].ProductionModifier << "\n";

            file << gameVariables.Cities[i].isProducing << "\n";
            file << gameVariables.Cities[i].isProducingUnit << "\n";

            file << gameVariables.Cities[i].FoodSurplus << "\n";
            file << gameVariables.Cities[i].FoodPerTurnFromTiles << "\n";
            file << gameVariables.Cities[i].FoodPerTurnFromCity << "\n";

            file << gameVariables.Cities[i].GoldPerTurn << "\n";
            file << gameVariables.Cities[i].GoldPerTurnFromCity << "\n";

            file << gameVariables.Cities[i].buildings.size() << "\n";

            for (unsigned int j = 0; j < gameVariables.Cities[i].buildings.size(); j++) {

                file << gameVariables.Cities[i].buildings[j] << "\n";

            }

            file << gameVariables.Cities[i].AvailableBuildingsToCreate.size() << "\n";

            for (unsigned int j = 0; j < gameVariables.Cities[i].AvailableBuildingsToCreate.size(); j++) {

                file << gameVariables.Cities[i].AvailableBuildingsToCreate[j] << "\n";

            }

            file << gameVariables.Cities[i].unitBeingProduced.parentCivilizationIndex << "\n";

            file << gameVariables.Cities[i].unitBeingProduced.name << "\n";

            file << gameVariables.Cities[i].buildingBeingProduced.name << "\n";

        }

        file.close();

    }
}

void Game::checkTileForResource (int civilizationIndex, int x, int y) {

    if (gameVariables.worldMap.WorldResourceMap[x][y].ResourceCode != gameVariables.worldMap.resources::NONE
        && gameVariables.worldMap.WorldTerritoryMap[x][y] == civilizationIndex+1) {

        gameVariables.Civilizations[civilizationIndex].resources.push_back(gameVariables.worldMap.WorldResourceMap[x][y]);

    }

}

void Game::pickResearchForHumanPlayer (int civilizationIndex) {

    std::vector<int> researchableTechIndices;

    for (unsigned int i = 0; i < gameVariables.Civilizations[civilizationIndex].technologiesToResearch.size(); i++) {

        if (sharedMethods::CivilizationHasPrerequisiteTechs(civilizationIndex,
        gameVariables.Civilizations[civilizationIndex].technologiesToResearch[i].researchName, gameVariables)) {

            researchableTechIndices.push_back(i);

        }

    }

    if (researchableTechIndices.size() > 0) {

        std::cout << "Choose a technology to research:";
        for (unsigned int i = 0; i < researchableTechIndices.size(); i++) {

            std::cout << "\n[" << i << "] " << gameVariables.Civilizations[civilizationIndex].technologiesToResearch[researchableTechIndices[i]].researchName;

        }

        int techIndex = sharedMethods::bindIntegerInputToRange(0, researchableTechIndices.size()-1, 0);

        gameVariables.Civilizations[civilizationIndex].technologyBeingResearched = gameVariables.Civilizations[civilizationIndex].technologiesToResearch[researchableTechIndices[techIndex]];

    } else {

        std::cout << "You have nothing left to research!" << std::endl;

    }

}

int Game::getCivilizationPopulation (int civilizationIndex) {

    int population = 0;

    for (unsigned int i = 0; i < gameVariables.Cities.size(); i++) {

        if (gameVariables.Cities[i].parentIndex == civilizationIndex) {

            population += gameVariables.Cities[i].Population;

        }

    }

    return population;

}

bool Game::startLocationIsValid (int x, int y) {

    int numberOfCivilizationsPositionIsFarAwayFrom = 0;

    if (gameVariables.Civilizations.size() >= 1) {

        for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

            if (sharedMethods::getDistance(x, y, gameVariables.Civilizations[i].startingX, gameVariables.Civilizations[i].startingY) > 5) {

                numberOfCivilizationsPositionIsFarAwayFrom++;

            }

        }

    }

    if (gameVariables.worldMap.featureMap[x][y] != gameVariables.worldMap.mapTiles::OCEAN
            && gameVariables.worldMap.featureMap[x][y] != gameVariables.worldMap.mapTiles::COAST
            && isLandTile (x, y) && returnLandTiles(x, y) > 2
            && numberOfCivilizationsPositionIsFarAwayFrom == gameVariables.Civilizations.size()) {

        return true;

    } else {

        return false;

    }

}

void Game::setNewCivilizationExploration () {

    int new_civilization_index = gameVariables.Civilizations.size()-1;

    for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {

        for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {

            if (sharedMethods::getDistance(gameVariables.Civilizations[new_civilization_index].startingX, gameVariables.Civilizations[new_civilization_index].startingY, i, j) <= 3) {

                gameVariables.Civilizations[new_civilization_index].WorldExplorationMap[i][j] = 1;

            } else {

                gameVariables.Civilizations[new_civilization_index].WorldExplorationMap[i][j] = 0;

            }

        }

    }

}

void Game::spawnInNewCivilization (std::vector<Civilization> civs) {

     bool loopSpawn = true;

     while (loopSpawn) {

        Civilization civ;

        int x = rand() % (gameVariables.worldMap.worldSize);
        int y = rand() % (gameVariables.worldMap.worldSize*4);


        if (startLocationIsValid(x, y)) {

                loopSpawn = false;

                std::cout << "Is Civilization #" << gameVariables.Civilizations.size() << " being played by a human? (Y/N)" << std::endl;

                char Choice;

                std::cin >> Choice;

                if (Choice == 'y' || Choice == 'Y') {

                    std::cout << "Pick a civilization to choose from." << std::endl;

                    for (int i = 0; i < civs.size(); i++) {

                        std::cout << "[" << i << "] " << civs[i].CivName << std::endl;

                    }

                    int choice = sharedMethods::bindIntegerInputToRange(0, civs.size()-1, 0);

                    civ = civs[choice];

                    civ.playedByHumans = true;

                } else {

                    int civIndex = rand() % civs.size();

                    civ = civs[civIndex];

                }

                civ.AvailableUnitsToCreate.push_back("Warrior");
                civ.AvailableUnitsToCreate.push_back("Explorer");
                civ.AvailableUnitsToCreate.push_back("Settler");

                civ.startingX = x; civ.startingY = y; gameVariables.Civilizations.push_back(civ);

                sharedMethods::foundCity(x, y, gameVariables.Civilizations.size() - 1, gameVariables);

                gameVariables.Cities [gameVariables.Cities.size () - 1].isCapital = true;

                loadTechnologiesFromFile("techs.sav", gameVariables.Civilizations.size() - 1);

                gameVariables.Civilizations[gameVariables.Civilizations.size() - 1].technologyBeingResearched.researchName = "";

                Unit explorer = gameVariables.Units[sharedMethods::getUnitIndexByName("Explorer", gameVariables)];
                explorer.position.setCoordinates(civ.startingX, civ.startingY);
                explorer.parentCivilizationIndex = gameVariables.Civilizations.size() - 1;

                gameVariables.UnitsInGame.push_back(explorer);

                Unit settler = gameVariables.Units[sharedMethods::getUnitIndexByName("Settler", gameVariables)];
                settler.position.setCoordinates(civ.startingX, civ.startingY);
                settler.parentCivilizationIndex = gameVariables.Civilizations.size() - 1;

                gameVariables.UnitsInGame.push_back(settler);

                Unit warrior = gameVariables.Units[sharedMethods::getUnitIndexByName("Warrior", gameVariables)];
                warrior.position.setCoordinates(civ.startingX, civ.startingY);
                warrior.parentCivilizationIndex = gameVariables.Civilizations.size() - 1;

                gameVariables.UnitsInGame.push_back(warrior); gameVariables.UnitsInGame.push_back (warrior);


        }

    }

}

void Game::generateCiv (int Civ) {

    std::vector<Civilization> civs;

    loadCivilizationsFromFile("civilizations.sav",civs);

    spawnInNewCivilization (civs);

    setNewCivilizationExploration ();

}

void Game::generateMinorCiv (int Civ) {

    std::vector<Civilization> civs;

    loadCivilizationsFromFile("minorCivilizations.sav",civs);

    spawnInNewCivilization (civs);

    setNewCivilizationExploration ();

}

int Game::returnNumberOfTilesExploredByCivilization (int civilizationIndex) {

    int tilesExplored = 0;

    for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {

        for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {

            if (gameVariables.Civilizations[civilizationIndex].WorldExplorationMap[i][j] == 1) { tilesExplored++; }

        }

    }

    return tilesExplored;

}

void Game::DisplayCivilizationExplorationProgress (int civilizationIndex) {


    int totalWorldArea = (gameVariables.worldMap.worldSize*gameVariables.worldMap.worldSize*4);

    double percentageOfWorldExplored = 100 * (double) returnNumberOfTilesExploredByCivilization (civilizationIndex) / totalWorldArea;

    std::cout << "\nYour civilization has explored " << percentageOfWorldExplored << "% of the world." << std::endl;
}

void Game::DisplayCitiesStatusesOwnedByCivilization (int civilizationIndex) {

    int cityIndex = 0; std::vector<int> cityIndices; double FoodNeededForNewCitizen;

    std::cout << "CITIES" << std::endl;

    for (unsigned int i = 0; i < gameVariables.Cities.size(); i++) {

        if (gameVariables.Cities[i].parentIndex == civilizationIndex) {

            FoodNeededForNewCitizen = pow((gameVariables.Cities[i].Population - 1), 2.1) + 5;

            double BarsToFill = ((gameVariables.Cities[i].FoodSurplus / FoodNeededForNewCitizen) * 10);

            std::cout << cityIndex+1 << ": " << gameVariables.Cities[i].cityName << " - " << gameVariables.Cities[i].position.x << ", " << gameVariables.Cities[i].position.y << " | FOOD YIELD : " << (gameVariables.Cities[i].FoodPerTurnFromCity + gameVariables.Cities[i].FoodPerTurnFromTiles) << " (" << gameVariables.Cities[i].FoodPerTurnFromTiles <<  ") | POPULATION : " << gameVariables.Cities[i].Population << " [";

            for (int j = 0; j < (int)BarsToFill; j++) {
                std::cout << "\033[32m*";
            }
            for (int j = 0; j < (int)(10-BarsToFill); j++) {
                std::cout << "\033[0m ";
            }
            std::cout << "\e[0m] | PRODUCTION: ";

            std::cout << gameVariables.Cities[i].ProductionPerTurn + gameVariables.Cities[i].ProductionFromTiles;

            std::cout << " | GPT: " << gameVariables.Cities[i].GoldPerTurn + gameVariables.Cities[i].GoldPerTurnFromCity << std::endl;

            std::cout << "  - Revolt Chance : " << gameVariables.Cities[i].chanceOfRevoltingPerTurnOutOf100 << "%" << std::endl;

            std::string productionString = "";

            if (gameVariables.Cities[i].isProducing == false) {

                productionString = "nothing";

            } else if (gameVariables.Cities[i].isProducingUnit == false) {

                productionString = gameVariables.Cities[i].buildingBeingProduced.name;

            } else {

                 productionString = gameVariables.Cities[i].unitBeingProduced.name;

            }

            std::cout << "  - The city is currently producing " << productionString;

            if (productionString == "nothing") {

                std::cout << "." << std::endl;

            } else if (gameVariables.Cities[i].isProducingUnit == false) {

                std::cout << " (" << gameVariables.Cities[i].Production << " / " << gameVariables.Cities[i].buildingBeingProduced.productionCost << ")" << std::endl;

            } else {

                std::cout << " (" << gameVariables.Cities[i].Production << " / " << gameVariables.Cities[i].unitBeingProduced.productionCost << ")" << std::endl;

            }

            cityIndex++; int y = i; cityIndices.push_back(y);

        }

    }

    char choice;

    std::cin >> choice;

    if (choice == 'p' || choice == 'P') {

        std::cout << "What city do you want to produce for?" << std::endl;

        int choice = 0;

        choice = sharedMethods::bindIntegerInputToRange (0, cityIndices.size()-1, 0);

        setCityProduction (cityIndices[choice]);

    }

    if (choice == 'b' || choice == 'B') {

        std::cout << "What city do you want to buy units in?" << std::endl;

        int choice = 0;

        choice = sharedMethods::bindIntegerInputToRange (0, cityIndices.size()-1, 0);

        buyUnits (civilizationIndex, cityIndices[choice]);

    }
}

void Game::showAvailableUnits (int civilizationIndex, int cityIndex, bool forProduction = false, bool forBuying = true) {

    std::vector<int> availableUnitIndices;


    for (int i = 0; i < gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate.size(); i++) {

        if (gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[i], gameVariables)].domain.name != "Land"
         && returnLandTiles (gameVariables.Cities[cityIndex].position.x, gameVariables.Cities[cityIndex].position.y) >= 8) {

            /// naval unit's can't be produced on land!

        } else {

            availableUnitIndices.push_back (i);

        }

    }

    std::cout << "UNITS AVAILABLE" << std::endl;

    if (forProduction == true) {

        for (unsigned int i = 0; i < availableUnitIndices.size(); i++) {

            std::cout << "[" << i << "] : " << gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[i]] << " | "
            << gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[i]], gameVariables)].productionCost << std::endl;

        }

    }

    if (forBuying == true) {

        for (unsigned int i = 0; i < availableUnitIndices.size(); i++) {

            std::cout << "[" << i << "] : " << gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[i]] << " | $"
            << gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[i]], gameVariables)].goldCost << std::endl;

        }

    }

}

void Game::showAvailableBuildings (int cityIndex) {

    std::cout << "BUILDINGS AVAILABLE" << std::endl;

    for (unsigned int i = 0; i < gameVariables.Cities[cityIndex].AvailableBuildingsToCreate.size(); i++) {

        std::cout << "[" << i << "] : " << gameVariables.Cities[cityIndex].AvailableBuildingsToCreate[i] << " | " << gameVariables.Buildings[sharedMethods::getBuildingIndexByName(gameVariables.Cities[cityIndex].AvailableBuildingsToCreate[i], gameVariables)].productionCost << std::endl;

    }

}

void Game::buyUnits (int civilizationIndex, int cityIndex) {

    std::vector<int> availableUnitIndices;

    showAvailableUnits (civilizationIndex, cityIndex);

    for (int i = 0; i < gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate.size(); i++) {

        if (gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[i], gameVariables)].domain.name != "Land"
         && returnLandTiles (gameVariables.Cities[cityIndex].position.x, gameVariables.Cities[cityIndex].position.y) >= 8) {

            /// naval unit's can't be produced on land!

        } else {

            availableUnitIndices.push_back (i);

        }

    }

    int choice = sharedMethods::bindIntegerInputToRange(0, availableUnitIndices.size()-1, 0);

    if (gameVariables.Civilizations[civilizationIndex].Gold >= gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[choice]], gameVariables)].goldCost) {

        Unit unit = gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[choice]], gameVariables)];

        unit.position.x = gameVariables.Cities[cityIndex].position.x;
        unit.position.y = gameVariables.Cities[cityIndex].position.y;
        unit.parentCivilizationIndex = civilizationIndex;

        gameVariables.UnitsInGame.push_back(unit);

        std::cout << "Bought a(n) " << unit.name << "!" << std::endl;

        gameVariables.Civilizations[civilizationIndex].Gold -= gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[choice]], gameVariables)].goldCost;

    } else {

        std::cout << "Not enough gold!" << std::endl;

    }
}

void Game::setUnitUpkeep (int civilizationIndex) {

    bool stillInputting = true;

    int upkeepCost = gameVariables.Civilizations[civilizationIndex].upkeepCostPerUnit;

    while (stillInputting) {

        std::cout << "Current unit upkeep is " << upkeepCost
            << ".\nNecessary upkeep costs for units to perform effectively is " << sharedMethods::returnBaseUnitUpkeep (civilizationIndex, gameVariables)
            << ".\nYou can spend up to " << sharedMethods::returnBaseUnitUpkeep (civilizationIndex, gameVariables) + 2 << " gold per turn on units for upkeep."
            << "Press [N] to input new upkeep value, or press [C] to confirm it." << std::endl;

        char input;

        std::cin >> input;

        if (input == 'n' || input == 'N') {

            upkeepCost = sharedMethods::bindIntegerInputToRange (0, sharedMethods::returnBaseUnitUpkeep (civilizationIndex, gameVariables) + 2,
                sharedMethods::returnBaseUnitUpkeep (civilizationIndex, gameVariables));

        } else if (input == 'c' || input == 'C') {

            gameVariables.Civilizations[civilizationIndex].upkeepCostPerUnit = upkeepCost;

            stillInputting = false;

        }

    }

}

void Game::editLoan (int civilizationIndex, Loan &loan) {

    bool isStillEditingTrade = true;

    while (isStillEditingTrade) {

        std::cout << "= LOAN ITEMS =" << std::endl;

        std::cout << "GOLD: " << loan.amountDue << std::endl;

        std::cout << "\n[Z] Edit your gold sum [E] Confirm loan" << std::endl;

        char characterInput;

        std::cin >> characterInput;

        if (characterInput == 'E' || characterInput == 'e') {

            isStillEditingTrade = false;

        }

        if (characterInput == 'Z' || characterInput == 'z') {

            int amountOfGold = 0;

            std::cout << "Please input the amount of gold you want to trade: ";

            amountOfGold = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[civilizationIndex].Gold, 0);

            loan.amountDue = amountOfGold;

        }
    }
}

void Game::offerLoan (int civilizationIndex) {

    int Choice = 0; std::vector<int> valid_civilization_indices;

    for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

        if (i != civilizationIndex && gameVariables.Civilizations[civilizationIndex].hasMetCivilization (i)) {

           valid_civilization_indices.push_back(i);

        }

    }

    if (valid_civilization_indices.size () >= 1) {

        for (unsigned int i = 0; i < valid_civilization_indices.size(); i++) {

            std::cout << "[" << i << "] " << gameVariables.Civilizations[valid_civilization_indices[i]].CivName << std::endl;

        }

        Choice = sharedMethods::bindIntegerInputToRange (0, valid_civilization_indices.size() -1, 0);

        Loan temp_loan;

        temp_loan.creditorCivilizationIndex = civilizationIndex;
        temp_loan.debtorCivilizationIndex = valid_civilization_indices[Choice];

        editLoan (civilizationIndex, temp_loan);

        gameVariables.activeLoans.push_back (temp_loan);

        sharedMethods::createNewLoanEventNotification (civilizationIndex, gameVariables, temp_loan);

    } else {

        std::cout << "There aren't any civilizations you can trade with!" << std::endl;

    }

}

void Game::openTradingMenu (int civilizationIndex) {

    std::cout << "-= TRADE MENU =- " << std::endl;

    std::cout << "\n[T]rade\n[L]oan" << std::endl;

    char temp_input;

    std::cin >> temp_input;

    if (temp_input == 't' || temp_input == 'T') {

        trade (civilizationIndex);

    }

    if (temp_input == 'l' || temp_input == 'L') {

        std::cout << "[R]equest a loan from another civilization, or [o]ffer a loan of your own." << std::endl;

        std::cin >> temp_input;

        if (temp_input == 'o' || temp_input == 'O') {

            offerLoan (civilizationIndex);

        }

    }

}

void Game::openDiplomacyMenu (int civilizationIndex) {

       std::cout << "Diplomacy" << std::endl;

        displayAlliances ();

        displayWars ();

        std::cout << "[O]ffer Alliance\n[D]eclare War" << std::endl;

        char Choice;

        std::cin >> Choice;

        if (Choice == 'o' || Choice == 'O') {

            playerRequestAlliance (civilizationIndex);

        }

        if (Choice == 'd' || Choice == 'D') {

            int Choice = 0; std::vector<int> valid_civilization_indices;

            for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

                if (i != civilizationIndex && gameVariables.Civilizations[civilizationIndex].hasMetCivilization (i)) {

                   valid_civilization_indices.push_back(i);

                }

            }

            if (valid_civilization_indices.size () >= 1) {

                for (unsigned int i = 0; i < valid_civilization_indices.size(); i++) {

                    std::cout << "[" << i << "] " << gameVariables.Civilizations[valid_civilization_indices[i]].CivName << std::endl;

                }

                int Choice = 0;

                Choice = sharedMethods::bindIntegerInputToRange(0,valid_civilization_indices.size()-1,0);

                declareWar (civilizationIndex, valid_civilization_indices[Choice]);

            }

    }

}

void Game::displayAlliances () {

    std::cout << "Alliances" << std::endl;
    for (unsigned int i = 0; i < gameVariables.alliances.size(); i++) {

        std::cout << "- " << gameVariables.alliances[i].name << std::endl;

        for (unsigned int j = 0; j < gameVariables.alliances[i].memberCivilizationIndices.size(); j++) {

            std::cout << "   " << gameVariables.Civilizations[gameVariables.alliances[i].memberCivilizationIndices[j]].CivName << std::endl;

        }

    }

}

void Game::displayWars () {

    std::cout << "Wars" << std::endl;
    for (unsigned int i = 0; i < gameVariables.wars.size(); i++) {

        std::cout << "- " << gameVariables.wars[i].name << std::endl;

        for (unsigned int j = 0; j < gameVariables.wars[i].offenderCivilizationIndices.size(); j++) {

            std::cout << "  " << gameVariables.Civilizations[gameVariables.wars[i].offenderCivilizationIndices[j]].CivName << std::endl;

        }

        std::cout << "  VS." << std::endl;

        for (unsigned int j = 0; j < gameVariables.wars[i].defenderCivilizationIndices.size(); j++) {

            std::cout << "  " << gameVariables.Civilizations[gameVariables.wars[i].defenderCivilizationIndices[j]].CivName << std::endl;

        }

    }

}

void Game::getPlayerChoiceAndReact (int civilizationIndex) {

    char Choice;

    std::cin >> Choice;

    if (Choice == characterActionCodes::EXPLORATION) {

        DisplayCivilizationExplorationProgress(civilizationIndex);

    }

    if (Choice == 'c' || Choice == 'C') {

        DisplayCitiesStatusesOwnedByCivilization(civilizationIndex);

        std::cin >> Choice;

    }

    if (Choice == 'T' || Choice == 't') {

        openTradingMenu (civilizationIndex);

    }

    if (Choice == 'F' || Choice == 'f') {

        std::cout << "Current focus : " << gameVariables.Civilizations[civilizationIndex].focus << std::endl;

        std::cout << "Change focus? [Y/N]" << std::endl;

        std::cin >> Choice;

        if (Choice == 'Y' || Choice == 'y') {

            std::cout << "[1] Construction\n[2] Economic\n[3] Military\n[4] Population" << std::endl;

            Choice = sharedMethods::bindIntegerInputToRange (1, 4, 1);

            switch (Choice) {

                case 1:
                    gameVariables.Civilizations[civilizationIndex].focus = "Construction";
                    break;
                case 2:
                    gameVariables.Civilizations[civilizationIndex].focus = "Economic";
                    break;
                case 3:
                    gameVariables.Civilizations[civilizationIndex].focus = "Military";
                    break;
                case 4:
                    gameVariables.Civilizations[civilizationIndex].focus = "Population";
                    break;

            }

        }

    }

    if (Choice == 'r' || Choice == 'R') {

        pickResearchForHumanPlayer (civilizationIndex);

    }

    if (Choice == 'd') {

        openDiplomacyMenu (civilizationIndex);

    }

    if (Choice == ';') { renderer.spectate(turnNumber, gameVariables); }

    if (Choice == 's') { saveGame ("save.save"); }

    if (Choice == 'g') {

        renderer.DisplayUnitGroupings (civilizationIndex, gameVariables);

        std::cin >> Choice;

        if (Choice == 'c') {

            std::cout << "Create new unit grouping\nEnter a name for the group: ";

            std::string name;

            std::cin.ignore();
            std::getline(std::cin, name);

            int rgbArray[3] = {rand() % 254 + 1,rand() % 254 + 1,rand() % 254 + 1};

            gameVariables.Civilizations[civilizationIndex].addNewGrouping (name, rgbArray);


        }

    }

    if (Choice == 'p') { setUnitUpkeep (civilizationIndex); }

    if (Choice == 'u' || Choice == 'U') {

        int x = 0; int militarypower = 0; std::vector<int> unitIndices;

        std::cout << "UNITS" << std::endl;

        for (unsigned int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

            if (gameVariables.UnitsInGame[i].parentCivilizationIndex == civilizationIndex) {

                std::cout << x << ": " << gameVariables.UnitsInGame[i].name << " - " << gameVariables.UnitsInGame[i].position.y << ", "
                << gameVariables.UnitsInGame[i].position.x << " | MOVEMENT LEFT : " << gameVariables.UnitsInGame[i].movementPoints << std::endl;
                x++; int y = i; unitIndices.push_back(y);

                militarypower += (gameVariables.UnitsInGame[i].combatStrength * gameVariables.UnitsInGame[i].combatStrength);

            }

        }

        std::cout << "Your Civilization's Military Power: " << militarypower << std::endl;

        std::cin >> Choice;

        if (Choice == 'a' && gameVariables.Civilizations[civilizationIndex].unitGroups.size() >= 1) {

            std::cout << "Add what unit?" << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            gameVariables.UnitsInGame[unitIndices[whichOne]].parentGroupingIndex = 0;

            std::cout << "Add to which grouping?" << std::endl;

            int grouping = sharedMethods::bindIntegerInputToRange(0, gameVariables.Civilizations[civilizationIndex].unitGroups.size()-1, 0);

            gameVariables.Civilizations[civilizationIndex].unitGroups[grouping].memberUnitIndices.push_back(unitIndices[whichOne]);

        }

        if (Choice == 'm' || Choice == 'M') {

            std::cout << "\nMove which unit? " << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            while (gameVariables.UnitsInGame[unitIndices[whichOne]].movementPoints > 0) {

                std::cout << "[1] move down [2] move right [3] move up [4] move left" << std::endl;

                int d;

                std::cin >> d;

                if (d == 1 && gameVariables.UnitsInGame[unitIndices[whichOne]].position.x < gameVariables.worldMap.worldSize) {

                    sharedMethods::moveUnit(gameVariables.UnitsInGame[unitIndices[whichOne]],
                        1,
                        0,
                        gameVariables);

                }

                if (d == 2 && gameVariables.UnitsInGame[unitIndices[whichOne]].position.y < gameVariables.worldMap.worldSize*4) {

                    sharedMethods::moveUnit(gameVariables.UnitsInGame[unitIndices[whichOne]],
                        0,
                        1,
                        gameVariables);

                }

                if (d == 3 && gameVariables.UnitsInGame[unitIndices[whichOne]].position.x > 0) {

                    sharedMethods::moveUnit(gameVariables.UnitsInGame[unitIndices[whichOne]],
                        -1,
                        0,
                        gameVariables);

                }

                if (d == 4 && gameVariables.UnitsInGame[unitIndices[whichOne]].position.y > 0) {

                    sharedMethods::moveUnit(gameVariables.UnitsInGame[unitIndices[whichOne]],
                        0,
                        -1,
                        gameVariables);

                }
            }
        }

        if (Choice == 'd') {

            std::cout << "\nView which unit's details? " << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            displayUnitDetails (gameVariables.UnitsInGame[unitIndices[whichOne]]);
        }

        if (Choice == 't' || Choice == 'T') {

            std::cout << "\nTrain which unit? " << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            gameVariables.UnitsInGame[unitIndices[whichOne]].isTraining = true;

        }

        if (Choice == 's' || Choice == 'S') {

            std::cout << "\nUse which unit's special ability? " << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            if (gameVariables.UnitsInGame[unitIndices[whichOne]].name == "Settler") {

                sharedMethods::foundCity (gameVariables.UnitsInGame[unitIndices[whichOne]].position.x,
                    gameVariables.UnitsInGame[unitIndices[whichOne]].position.y, civilizationIndex,
                    gameVariables);

                gameVariables.UnitsInGame.erase (gameVariables.UnitsInGame.begin() + unitIndices[whichOne]);

            }

        }

        if (Choice == 'c' || Choice == 'C') {

            std::cout << "\nAttack with which unit? " << std::endl;

            int whichOne;

            whichOne = sharedMethods::bindIntegerInputToRange(0, unitIndices.size()-1, 0);

            std::vector<int> validEnemyUnitIndices;

            for (unsigned int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

                if (gameVariables.UnitsInGame[i].parentCivilizationIndex != civilizationIndex
                    && sharedMethods::areCivsAtWar (civilizationIndex, gameVariables.UnitsInGame[i].parentCivilizationIndex, gameVariables)
                    && sharedMethods::getDistance(gameVariables.UnitsInGame[unitIndices[whichOne]].position.x,
                        gameVariables.UnitsInGame[unitIndices[whichOne]].position.y,
                        gameVariables.UnitsInGame[i].position.x, gameVariables.UnitsInGame[i].position.y) < (gameVariables.UnitsInGame[unitIndices[whichOne]].range+1)) {

                    validEnemyUnitIndices.push_back (i);

                }

            }

            if (validEnemyUnitIndices.size() >= 1) {

                std::cout << "Attack which unit?" << std::endl;

                for (unsigned int i = 0; i < validEnemyUnitIndices.size(); i++) {

                    std::cout << "[" << i << "] " << gameVariables.UnitsInGame[validEnemyUnitIndices[i]].name << " : "
                        << gameVariables.Civilizations[gameVariables.UnitsInGame[validEnemyUnitIndices[i]].parentCivilizationIndex].CivName << " ("
                        << gameVariables.UnitsInGame[validEnemyUnitIndices[i]].position.x << ", "
                        << gameVariables.UnitsInGame[validEnemyUnitIndices[i]].position.y << ") - "
                        << gameVariables.UnitsInGame[validEnemyUnitIndices[i]].health << " HP" << std::endl;

                }

                Choice = sharedMethods::bindIntegerInputToRange (0, validEnemyUnitIndices.size() - 1, 0);

                if (sharedMethods::getDistance(gameVariables.UnitsInGame[unitIndices[whichOne]].position.x,
                    gameVariables.UnitsInGame[unitIndices[whichOne]].position.y,
                    gameVariables.UnitsInGame[validEnemyUnitIndices[Choice]].position.x,
                    gameVariables.UnitsInGame[validEnemyUnitIndices[Choice]].position.y) < 2
                    && gameVariables.UnitsInGame[validEnemyUnitIndices[Choice]].isRanged == false) {

                    combat (gameVariables.UnitsInGame[unitIndices[whichOne]], gameVariables.UnitsInGame[validEnemyUnitIndices[Choice]]);

                } else {

                    rangedCombat (gameVariables.UnitsInGame[unitIndices[whichOne]], gameVariables.UnitsInGame[validEnemyUnitIndices[Choice]]);

                }

            } else {

                std::cout << "You can't attack any units with this unit!" << std::endl;

            }

        }

    }

    if (Choice == 'w') {

        for (unsigned int i = 0; i < gameVariables.Civilizations[civilizationIndex].resources.size(); i++) {

            std::cout << resourceNames[gameVariables.Civilizations[civilizationIndex].resources[i].ResourceCode] << " : " << gameVariables.Civilizations[civilizationIndex].resources[i].AmountOfResource << std::endl;

        }

    }

    if (Choice == 'e' || Choice == 'E') {

        if (gameVariables.Civilizations[civilizationIndex].technologyBeingResearched.researchName != "") {

            loopVariable = false; std::cout << "Ending your turn..." << std::endl;

        } else {

            std::cout << "You need to pick a technology to research!" << std::endl;

        }
    }

    if (Choice == 'Q') {

        std::cout << "Exiting game..." << std::endl;

        loopVariable = false;
        gameLoopVariable = false;

    }

}

void Game::produceUnits (int cityIndex, int civilizationIndex) {

    showAvailableUnits (civilizationIndex, cityIndex, true, false);

    std::vector<int> availableUnitIndices;

    showAvailableUnits (civilizationIndex, cityIndex);

    for (int i = 0; i < gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate.size(); i++) {

        if (gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[i], gameVariables)].domain.name != "Land"
         && returnLandTiles (gameVariables.Cities[cityIndex].position.x, gameVariables.Cities[cityIndex].position.y) >= 8) {

            /// naval unit's can't be produced on land!

        } else {

            availableUnitIndices.push_back (i);

        }

    }

    int choice = sharedMethods::bindIntegerInputToRange(0, availableUnitIndices.size()-1, 0);

    Unit unit = gameVariables.Units[sharedMethods::getUnitIndexByName(gameVariables.Civilizations[civilizationIndex].AvailableUnitsToCreate[availableUnitIndices[choice]], gameVariables)];

    gameVariables.Cities[cityIndex].unitBeingProduced = unit;

    gameVariables.Cities[cityIndex].isProducing = true;
    gameVariables.Cities[cityIndex].isProducingUnit = true;

}

void Game::produceBuilding (int cityIndex, int civilizationIndex) {

    showAvailableBuildings (civilizationIndex);

    int choice = sharedMethods::bindIntegerInputToRange(0, gameVariables.Cities[cityIndex].AvailableBuildingsToCreate.size()-1, 0);

    Building building = gameVariables.Buildings[sharedMethods::getBuildingIndexByName(gameVariables.Cities[cityIndex].AvailableBuildingsToCreate[choice], gameVariables)];

    gameVariables.Cities[cityIndex].buildingBeingProduced = building;

    gameVariables.Cities[cityIndex].isProducing = true;
    gameVariables.Cities[cityIndex].isProducingUnit = false;

}

void Game::setCityProduction (int cityIndex) {

    std::cout << "What do you want to produce in the city of " << gameVariables.Cities[cityIndex].cityName << "?\n[1] Units\n[2] Buildings" << std::endl;

    int choice = sharedMethods::bindIntegerInputToRange(1,2,1);

    if (choice == 1) {

        produceUnits (cityIndex, gameVariables.Cities[cityIndex].parentIndex);

    }

    if (choice == 2) {

        produceBuilding (cityIndex, gameVariables.Cities[cityIndex].parentIndex);

    }

    gameVariables.Cities[cityIndex].Production = 0;
}

int Game::getTerrainCodeUnitIsOn (Unit &unit) {

    return gameVariables.worldMap.featureMap[unit.position.x][unit.position.y];

}

void Game::updateUnitTraining () {

    for (int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

        if (gameVariables.UnitsInGame[i].isTraining) {

            switch (getTerrainCodeUnitIsOn(gameVariables.UnitsInGame[i])) {

                case 2: // grassland
                    gameVariables.UnitsInGame[i].grasslandModifier.experience += 0.08;

                    if (gameVariables.UnitsInGame[i].grasslandModifier.experience >= 1.00) {

                            gameVariables.UnitsInGame[i].grasslandModifier.experience = 1.00;

                            gameVariables.UnitsInGame[i].isTraining = false;

                            gameVariables.UnitsInGame[i].accolades.push_back ("[#] - Plains Regulars");

                    }

                break;
                case 3: // mountain
                    gameVariables.UnitsInGame[i].mountainModifier.experience += 0.08;

                    if (gameVariables.UnitsInGame[i].mountainModifier.experience >= 1.00) {

                            gameVariables.UnitsInGame[i].mountainModifier.experience = 1.00;

                            gameVariables.UnitsInGame[i].isTraining = false;

                            gameVariables.UnitsInGame[i].accolades.push_back ("[^] - Mountaineers");

                    }

                break;
                case 4: // forest
                    gameVariables.UnitsInGame[i].forestModifier.experience += 0.08;

                    if (gameVariables.UnitsInGame[i].forestModifier.experience >= 1.00) {

                            gameVariables.UnitsInGame[i].forestModifier.experience = 1.00;

                            gameVariables.UnitsInGame[i].isTraining = false;

                            gameVariables.UnitsInGame[i].accolades.push_back ("[*] - Rangers");

                    }

                break;
                case 6: // snow
                    gameVariables.UnitsInGame[i].snowModifier.experience += 0.08;

                    if (gameVariables.UnitsInGame[i].snowModifier.experience >= 1.00) {

                            gameVariables.UnitsInGame[i].snowModifier.experience = 1.00;

                            gameVariables.UnitsInGame[i].isTraining = false;

                            gameVariables.UnitsInGame[i].accolades.push_back ("[s] - Snowtroopers");

                    }

                break;
                case 7: // desert
                    gameVariables.UnitsInGame[i].desertModifier.experience += 0.08;

                     if (gameVariables.UnitsInGame[i].snowModifier.experience >= 1.00) {

                            gameVariables.UnitsInGame[i].snowModifier.experience = 1.00;

                            gameVariables.UnitsInGame[i].isTraining = false;

                            gameVariables.UnitsInGame[i].accolades.push_back ("[.] - Desert Troopers");

                    }

                break;

            }

        }

    }

}

void Game::requestAlliance (int civilizationIndex, int targetCivilizationIndex) {

    AllianceEvent *alliance = new AllianceEvent;

    alliance->EventName = "Alliance Request from " + gameVariables.Civilizations[civilizationIndex].CivName;

    alliance->EventMessage = "The wise leaders of " + gameVariables.Civilizations[civilizationIndex].CivName + " recognize that an alliance with us would be beneficial and request a formal alliance.";

    alliance->ResponseChoices.push_back("Accept Request");

    alliance->ResponseChoices.push_back("Deny Request");

    alliance->initializerCivilization = civilizationIndex; alliance->targetCivilizationIndex = targetCivilizationIndex;

    gameVariables.gameEvents.push_back (alliance);

}

void Game::playerRequestAlliance (int civilizationIndex) {

    int Choice = 0; std::vector<int> valid_civilization_indices;

    for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

        if (gameVariables.Civilizations[civilizationIndex].hasMetCivilization (i)) {

           valid_civilization_indices.push_back (i);

        }

    }

    if (valid_civilization_indices.size() >= 1) {

        std::cout << "Request an alliance with what Civilization?" << std::endl;

        for (unsigned int a = 0; a < valid_civilization_indices.size(); a++) {

            if (valid_civilization_indices [a] != civilizationIndex) {

                std::cout << "[" << a << "] " << gameVariables.Civilizations[valid_civilization_indices[a]].CivName << std::endl;

            }
        }

        Choice = sharedMethods::bindIntegerInputToRange(0,valid_civilization_indices.size()-1,0);

        requestAlliance (civilizationIndex, valid_civilization_indices[Choice]);

    } else {

        std::cout << "You haven't met any other civilizations yet!" << std::endl;

    }

}

void Game::displayCivilizationResources (int civilizationIndex) {

    for (int i = 0; i < gameVariables.Civilizations[civilizationIndex].resources.size(); i++) {

        std::cout << "[" << i << "] " << resourceNames[gameVariables.Civilizations[civilizationIndex].resources[i].ResourceCode] << " x" << gameVariables.Civilizations[civilizationIndex].resources[i].AmountOfResource << std::endl;

    }

}

void Game::displayUnitDetails (Unit unit) {

    std::cout << unit.name << " - " << unit.position.x << ", " << unit.position.y << std::endl;

    if (unit.isTraining) { std::cout << "Unit is training!" << std::endl; }

    std::cout << "HEALTH: " << unit.health << " / " << unit.maxHealth << std::endl;
    std::cout << "TERRAIN MODIFIERS\n~ Grassland\n  ~ Attack " << unit.grasslandModifier.attackModifier*100
        << " | Defense " << unit.grasslandModifier.defenseModifier*100 << " XP " << unit.grasslandModifier.experience*100 << "%" << std::endl;
    std::cout << "~ Forest\n  ~ Attack " << unit.forestModifier.attackModifier*100
        << " | Defense " << unit.forestModifier.defenseModifier*100 << " XP " << unit.forestModifier.experience*100 << "%" <<  std::endl;
    std::cout << "~ Mountain\n  ~ Attack " << unit.mountainModifier.attackModifier*100
        << " | Defense " << unit.mountainModifier.defenseModifier*100 << " XP " << unit.mountainModifier.experience*100 << "%" <<  std::endl;
    std::cout << "~ Desert\n  ~ Attack " << unit.desertModifier.attackModifier*100
        << " | Defense " << unit.desertModifier.defenseModifier*100 << " XP " << unit.desertModifier.experience*100 << "%" <<  std::endl;
    std::cout << "~ Snow\n  ~ Attack " << unit.snowModifier.attackModifier*100
        << " | Defense " << unit.snowModifier.defenseModifier*100 << " XP " << unit.snowModifier.experience*100 << "%" <<  std::endl;

    std::cout << "ACCOLADES:" << std::endl;

    for (unsigned int i = 0; i < unit.accolades.size(); i++) {

        std::cout << unit.accolades[i] << std::endl;

    }

    std::cin.ignore();
}

int Game::inputResourcesToTrade (int civilizationIndex, Trade &trade) {

    displayCivilizationResources (civilizationIndex);

    int selectedResourceIndex = 0;

    selectedResourceIndex = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[civilizationIndex].resources.size()-1, 0);

}

bool Game::resourceIsNotAlreadyBeingTraded (int civilizationIndex, int resourceIndex, std::vector<Resource> resources) {

    for (unsigned int i = 0; i < resources.size(); i++) {

        if (gameVariables.Civilizations[civilizationIndex].resources[resourceIndex].ResourceCode == resources[i].ResourceCode) {

            return false;

        }

    }

    return true;

}

void Game::editTrade (int civilizationIndex, Trade &trade, int Choice) {

    bool isStillEditingTrade = true;

    while (isStillEditingTrade) {

        std::cout << "= YOUR ITEMS =" << std::endl;

        std::cout << "GOLD: " << trade.goldSumFromTrader << "\nGPT: " << trade.GPTFromTrader << std::endl;

        std::cout << "RESOURCES: " << std::endl;

        for (int i = 0; i < trade.resourcesFromTrader.size(); i++) {

            std::cout << resourceNames[trade.resourcesFromTrader[i].ResourceCode] << std::endl;

        }

        std::cout << "= THEIR ITEMS =" << std::endl;

        std::cout << "GOLD: " << trade.goldSumFromRecipient << "\nGPT: " << trade.GPTFromRecipient << std::endl;

        std::cout << "RESOURCES: " << std::endl;

        for (int i = 0; i < trade.resourcesFromRecipient.size(); i++) {

            std::cout << resourceNames[trade.resourcesFromRecipient[i].ResourceCode] << std::endl;

        }

        std::cout << "\n[Z] Edit your gold sum [X] Edit their gold sum [C] Edit your resources to trade [V] Edit their resources to trade\n[B] Edit your GPT [N] Edit their GPT [E] Confirm trade" << std::endl;

        char characterInput;

        std::cin >> characterInput;

        if (characterInput == 'E' || characterInput == 'e') {

            isStillEditingTrade = false;

        }

        if (characterInput == 'Z' || characterInput == 'z') {

            int amountOfGold = 0;

            std::cout << "Please input the amount of gold you want to trade: ";

            amountOfGold = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[civilizationIndex].Gold, 0);

            trade.goldSumFromTrader = amountOfGold;

        }

        if (characterInput == 'X' || characterInput == 'x') {

            int amountOfGold = 0;

            std::cout << "Please input the amount of gold you want them to trade: ";

            amountOfGold = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[Choice].Gold, 0);

            trade.goldSumFromRecipient = amountOfGold;

        }

        if (characterInput == 'C' || characterInput == 'c') {

            std::cout << "[R]eset Traded Resources or [A]dd New Resources" << std::endl;

            std::cin >> characterInput;

            if (characterInput == 'R' || characterInput == 'r') {

                trade.resourcesFromTrader.clear();

            }

            if (characterInput == 'A' || characterInput == 'a') {


                int index = inputResourcesToTrade (civilizationIndex, trade);

                if (gameVariables.Civilizations[civilizationIndex].resources.size() > 0 && resourceIsNotAlreadyBeingTraded(civilizationIndex, index, trade.resourcesFromTrader)) {

                    trade.resourcesFromTrader.push_back (gameVariables.Civilizations[civilizationIndex].resources[index]);

                }

            }

        }

        if (characterInput == 'V' || characterInput == 'v') {

            std::cout << "[R]eset Traded Resources or [A]dd New Resources" << std::endl;

            std::cin >> characterInput;

            if (characterInput == 'R' || characterInput == 'r') {

                trade.resourcesFromRecipient.clear();

            }

            if (characterInput == 'A' || characterInput == 'a') {


                int index = inputResourcesToTrade (Choice, trade);

                if (gameVariables.Civilizations[Choice].resources.size() > 0 && resourceIsNotAlreadyBeingTraded(Choice, index, trade.resourcesFromRecipient)) {

                    trade.resourcesFromRecipient.push_back (gameVariables.Civilizations[Choice].resources[index]);

                }

            }

        }

        if (characterInput == 'B' || characterInput == 'b') {

            int amountOfGoldPerTurn = 0;

            std::cout << "Please input the amount of gold per turn you want to trade: ";

            amountOfGoldPerTurn = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[civilizationIndex].GoldPerTurn, 0);

            trade.GPTFromTrader = amountOfGoldPerTurn;

        }

        if (characterInput == 'N' || characterInput == 'n') {

            int amountOfGoldPerTurn = 0;

            std::cout << "Please input the amount of gold per turn you want them to trade. (They have " << gameVariables.Civilizations[Choice].GoldPerTurn << "): ";

            amountOfGoldPerTurn = sharedMethods::bindIntegerInputToRange (0, gameVariables.Civilizations[Choice].GoldPerTurn, 0);

            trade.GPTFromRecipient = amountOfGoldPerTurn;

        }

    }

}

void Game::trade (int civilizationIndex) {

    int Choice = 0; std::vector<int> valid_civilization_indices;

    std::cout << "Trade with what Civilization?" << std::endl;

    for (unsigned int i = 0; i < gameVariables.Civilizations.size(); i++) {

        if (i != civilizationIndex && gameVariables.Civilizations[civilizationIndex].hasMetCivilization (i)) {

            valid_civilization_indices.push_back (i);

        }

    }

    if (valid_civilization_indices.size () >= 1) {

        for (unsigned int i = 0; i < valid_civilization_indices.size(); i++) {

            std::cout << "[" << i << "] " << gameVariables.Civilizations[valid_civilization_indices[i]].CivName << std::endl;

        }

        Choice = sharedMethods::bindIntegerInputToRange (0, valid_civilization_indices.size() -1, 0);

        Trade trade;

        trade.recipientIndex = Choice;
        trade.traderIndex = civilizationIndex;

        editTrade (civilizationIndex, trade, valid_civilization_indices[Choice]);

        gameVariables.trades.push_back (trade);

    }

}

void Game::loop () {

    gameLoopVariable = true;

    for (int civilizationIndex = 0; civilizationIndex < gameVariables.Civilizations.size(); civilizationIndex++) {

        GameUpdater::updateResources (civilizationIndex, gameVariables);

    }

    while (gameLoopVariable == true){

        updateUnitTraining ();

        for (int civilizationIndex = 0; civilizationIndex < gameVariables.Civilizations.size(); civilizationIndex++) {

            loopVariable = true;

            GameUpdater::updateForCivilization (civilizationIndex, gameVariables, ai);

            sharedMethods::checkIfCivilizationHasMetNewCivilization (civilizationIndex, gameVariables);

            gameVariables.Civilizations[civilizationIndex].Gold += gameVariables.Civilizations[civilizationIndex].GoldPerTurn;

            if (gameVariables.Civilizations[civilizationIndex].playedByHumans) {

                while (loopVariable == true) {

                    GameUpdater::UpdateCivilizationExploredTerritory(civilizationIndex, gameVariables);

                    renderer.render(civilizationIndex, turnNumber, gameVariables);

                    getPlayerChoiceAndReact(civilizationIndex);

                }

            } else {

                ai.think(civilizationIndex, gameVariables);
                ai.moveAllUnitsBelongingToCiv(civilizationIndex, gameVariables);

            }

            gameVariables.Civilizations[civilizationIndex].researchPoints += (Game::getCivilizationPopulation(civilizationIndex) * gameVariables.Civilizations[civilizationIndex].ScienceRate);

            GameUpdater::updateResearch (civilizationIndex, gameVariables);

        }

        turnNumber++;

        GameUpdater::removeEliminatedCivilizations (gameVariables);

        GameUpdater::updateCities (gameVariables);

        GameUpdater::UpdateAllUnitsMovement (gameVariables);

        GameUpdater::updateRevolts (gameVariables);

    }

}

double Game::calculateUnitAttackingModifier (Unit &attacker, Unit &defender) {

    switch (getTerrainCodeUnitIsOn(defender)) {

    case gameVariables.worldMap.mapTiles::GRASSLAND :

        return attacker.grasslandModifier.attackModifier
            * gameVariables.Civilizations[attacker.parentCivilizationIndex].unitAttackModifier
            * (0.5 + (0.5 * attacker.grasslandModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::FOREST :

        return attacker.forestModifier.attackModifier
            * gameVariables.Civilizations[attacker.parentCivilizationIndex].unitAttackModifier
            * (0.5 + (0.5 * attacker.forestModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::DESERT :

        return attacker.desertModifier.attackModifier
            * gameVariables.Civilizations[attacker.parentCivilizationIndex].unitAttackModifier
            * (0.5 + (0.5 * attacker.desertModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::MOUNTAIN :

        return attacker.mountainModifier.attackModifier
            * gameVariables.Civilizations[attacker.parentCivilizationIndex].unitAttackModifier
            * (0.5 + (0.5 * attacker.mountainModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::SNOW :

        return attacker.snowModifier.attackModifier
            * gameVariables.Civilizations[attacker.parentCivilizationIndex].unitAttackModifier
            * (0.5 + (0.5 * attacker.snowModifier.experience));

    break;
    default :

        return 1.00;

    break;

    }

}

double Game::calculateUnitDefendingModifier (Unit &defender) {

    switch (getTerrainCodeUnitIsOn(defender)) {

    case gameVariables.worldMap.mapTiles::GRASSLAND :

        return defender.grasslandModifier.defenseModifier
            * gameVariables.Civilizations[defender.parentCivilizationIndex].unitDefenseModifier
            * (0.5 + (0.5 * defender.grasslandModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::FOREST :

        return defender.forestModifier.defenseModifier
            * gameVariables.Civilizations[defender.parentCivilizationIndex].unitDefenseModifier
            * (0.5 + (0.5 * defender.forestModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::DESERT :

        return defender.desertModifier.defenseModifier
            * gameVariables.Civilizations[defender.parentCivilizationIndex].unitDefenseModifier
            * (0.5 + (0.5 * defender.desertModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::MOUNTAIN :

        return defender.mountainModifier.defenseModifier
            * gameVariables.Civilizations[defender.parentCivilizationIndex].unitDefenseModifier
            * (0.5 + (0.5 * defender.mountainModifier.experience));

    break;
    case gameVariables.worldMap.mapTiles::SNOW :

        return defender.snowModifier.defenseModifier
            * gameVariables.Civilizations[defender.parentCivilizationIndex].unitDefenseModifier
            * (0.5 + (0.5 * defender.snowModifier.experience));

    break;
    default :

        return 1.00;

    break;

    }

}

int Game::returnUnitIndexFromPosition (int civilizationIndex, int x, int y) {

    for (unsigned int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

        if (gameVariables.UnitsInGame[i].position.x == x && gameVariables.UnitsInGame[i].position.y == y
            && gameVariables.UnitsInGame[i].parentCivilizationIndex == civilizationIndex) {

            return i;

        }

    }

}

void Game::displayLikelyCombatOutcome (Unit &attacker, Unit &defender) {

    double attackingModifier = calculateUnitAttackingModifier (attacker, defender),
        defenseModifier = calculateUnitDefendingModifier (defender);

    unsigned int attackerMaxDamage = ((attacker.combatStrength * attacker.health * attackingModifier * 2.6) / (defender.combatStrength * defender.health * defenseModifier)) + 1;
    unsigned int defenderMaxDamage = ((defender.combatStrength * defender.health * defenseModifier * 2.6) / (attacker.combatStrength * attacker.health * attackingModifier)) + 1;


    std::cout << attacker.name << " - " << gameVariables.Civilizations[attacker.parentCivilizationIndex].CivName << "       vs.      " << defender.name
        << " - " << gameVariables.Civilizations[defender.parentCivilizationIndex].CivName << std::endl;

    std::cout << "HP " << attacker.health << " | " << defender.health << std::endl;
    std::cout << "STRENGTH " << attacker.combatStrength << " | " << defender.combatStrength << std::endl;

    std::cout << "MODIFIER for Attacker : " << calculateUnitAttackingModifier(attacker, defender)*100 << "%" << std::endl;
    std::cout << "MODIFIER for Defender : " << calculateUnitDefendingModifier(defender)*100 << "%" << std::endl;

    std::cout << "Maximum possible damage : \n" << attackerMaxDamage << " | " << defenderMaxDamage << std::endl;

}

void Game::combat (Unit &attacker, Unit &defender) {

    displayLikelyCombatOutcome (attacker, defender);

    double attackingModifier = calculateUnitAttackingModifier (attacker, defender),
        defenseModifier = calculateUnitDefendingModifier (defender);

    unsigned int attackerMaxDamage = ((attacker.combatStrength * attacker.health * attackingModifier * 2.6) / (defender.combatStrength * defender.health * defenseModifier)) + 1;
    unsigned int defenderMaxDamage = ((defender.combatStrength * defender.health * defenseModifier * 2.6) / (attacker.combatStrength * attacker.health * attackingModifier)) + 1;

    int attackerDamage = rand () % attackerMaxDamage;
    int defenderDamage = rand () % defenderMaxDamage;

    attacker.health -= defenderDamage;
    defender.health -= attackerDamage;

    std::cout << attacker.name << " of " << gameVariables.Civilizations[attacker.parentCivilizationIndex].CivName
        << " dealt " << attackerDamage << " to the defender" << std::endl;

    std::cout << defender.name << " of " << gameVariables.Civilizations[defender.parentCivilizationIndex].CivName
        << " dealt " << defenderDamage << " to the attacker" << std::endl;

    giveUnitsCombatExperience (attacker, defender);

    if (attacker.health <= 0) {

        int unitIndex = returnUnitIndexFromPosition (attacker.parentCivilizationIndex, attacker.position.x, attacker.position.y);

        gameVariables.UnitsInGame.erase (gameVariables.UnitsInGame.begin() + unitIndex);

        createUnitDeathNotification (attacker, defender, true);

        removeDeadUnitFromGrouping (attacker.parentCivilizationIndex, unitIndex);

    }

    if (defender.health <= 0) {

        int unitIndex = returnUnitIndexFromPosition (defender.parentCivilizationIndex, defender.position.x, defender.position.y);

        gameVariables.UnitsInGame.erase (gameVariables.UnitsInGame.begin() + unitIndex);

        createUnitDeathNotification (defender, attacker, false);

        removeDeadUnitFromGrouping (defender.parentCivilizationIndex, unitIndex);

    }

}

void Game::rangedCombat (Unit &attacker, Unit &defender) {

    double attackingModifier = calculateUnitAttackingModifier (attacker, defender),
        defenseModifier = calculateUnitDefendingModifier (defender);

    unsigned int attackerMaxDamage = ((attacker.rangedCombat * attacker.health * attackingModifier * 1.5) / (defender.rangedCombat * defender.health * defenseModifier)) + 1;
    unsigned int defenderMaxDamage = ((defender.rangedCombat * defender.health * defenseModifier * 1.5) / (attacker.rangedCombat * attacker.health * attackingModifier)) + 1;

    int attackerDamage = rand () % attackerMaxDamage;
    int defenderDamage = rand () % defenderMaxDamage;

    attacker.health -= defenderDamage;

    if (defender.isRanged) {

        defender.health -= attackerDamage;

        std::cout << defender.name << " of " << gameVariables.Civilizations[defender.parentCivilizationIndex].CivName
            << " dealt " << defenderDamage << " to the attacker" << std::endl;


    }

    std::cout << attacker.name << " of " << gameVariables.Civilizations[attacker.parentCivilizationIndex].CivName
        << " dealt " << attackerDamage << " to the defender" << std::endl;

    if (attacker.health <= 0) {

        int unitIndex = returnUnitIndexFromPosition (attacker.parentCivilizationIndex, attacker.position.x, attacker.position.y);

        gameVariables.UnitsInGame.erase (gameVariables.UnitsInGame.begin() + unitIndex);

        createUnitDeathNotification (attacker, defender, true);

        removeDeadUnitFromGrouping (attacker.parentCivilizationIndex, unitIndex);

    }

    if (defender.health <= 0) {

        int unitIndex = returnUnitIndexFromPosition (defender.parentCivilizationIndex, defender.position.x, defender.position.y);

        gameVariables.UnitsInGame.erase (gameVariables.UnitsInGame.begin() + unitIndex);

        createUnitDeathNotification (attacker, defender, false);

        removeDeadUnitFromGrouping (defender.parentCivilizationIndex, unitIndex);

    }

}

void Game::removeDeadUnitFromGrouping (int civilizationIndex, int unitIndex) {

    for (unsigned int i = 0; i < gameVariables.Civilizations [civilizationIndex].unitGroups.size (); i++) {

        for (unsigned int j = 0; j < gameVariables.Civilizations [civilizationIndex].unitGroups [i].memberUnitIndices.size (); j++) {

            if (gameVariables.Civilizations [civilizationIndex].unitGroups [i].memberUnitIndices [j] == unitIndex) {

                j = gameVariables.Civilizations [civilizationIndex].unitGroups [i].memberUnitIndices.size ();

                gameVariables.Civilizations [civilizationIndex].unitGroups [i].memberUnitIndices.erase (
                    gameVariables.Civilizations [civilizationIndex].unitGroups [i].memberUnitIndices.begin () + j);

            }

        }

    }

}

void Game::createUnitDeathNotification (Unit &target, Unit &opponent, bool isAttacking) {

        Event *unit_died = new Event;

        unit_died->EventName = "Unit died in battle";

        if (isAttacking) {

            unit_died->EventMessage = "Your " + target.name + " died attacking a " + opponent.name + ".";

        } else {

            unit_died->EventMessage = "Your " + target.name + " died defending against a " + opponent.name + ".";

        }

        unit_died->ResponseChoices.push_back ("Okay.");

        unit_died->targetCivilizationIndex = target.parentCivilizationIndex;

        gameVariables.gameEvents.push_back (unit_died);

}

void Game::giveUnitsCombatExperience (Unit &attacker, Unit &defender) {

    switch (getTerrainCodeUnitIsOn(defender)) {

        case gameVariables.worldMap.mapTiles::GRASSLAND :

            attacker.grasslandModifier.experience += 0.1;
            defender.grasslandModifier.experience += 0.1;

        break;

        case gameVariables.worldMap.mapTiles::FOREST :

            attacker.forestModifier.experience += 0.1;
            defender.forestModifier.experience += 0.1;

        break;

        case gameVariables.worldMap.mapTiles::MOUNTAIN :

            attacker.mountainModifier.experience += 0.1;
            defender.mountainModifier.experience += 0.1;

        break;

        case gameVariables.worldMap.mapTiles::SNOW :

            attacker.snowModifier.experience += 0.1;
            defender.snowModifier.experience += 0.1;

        break;

        case gameVariables.worldMap.mapTiles::DESERT :

            attacker.desertModifier.experience += 0.1;
            defender.desertModifier.experience += 0.1;

        break;


    }

}

void Game::declareWar (int civilizationIndex, int targetCivilizationIndex) {

    if (!sharedMethods::areCivsAtWar(civilizationIndex, targetCivilizationIndex, gameVariables)
        && gameVariables.Civilizations[civilizationIndex].hasMetCivilization (targetCivilizationIndex)) {

        gameVariables.Civilizations[civilizationIndex].isAtWar = true;
        gameVariables.Civilizations[targetCivilizationIndex].isAtWar = true;

        War new_war;

        new_war.offenderCivilizationIndices.push_back (civilizationIndex);

        new_war.defenderCivilizationIndices.push_back (targetCivilizationIndex);

        new_war.name = gameVariables.Civilizations[civilizationIndex].CivilizationAdjective + "-"
            + gameVariables.Civilizations[targetCivilizationIndex].CivilizationAdjective + " War";

        bringBelligerentAlliesIntoWar (new_war, civilizationIndex);

        bringBelligerentAlliesIntoWar (new_war, targetCivilizationIndex);

        gameVariables.wars.push_back (new_war);

        Event *war_started = new Event;

        war_started->EventName = "Declaration of War";

        war_started->EventMessage = gameVariables.Civilizations[civilizationIndex].CivName + " has declared war on you!";

        war_started->ResponseChoices.push_back ("I knew they couldn't be trusted!");

        war_started->targetCivilizationIndex = targetCivilizationIndex;

        gameVariables.gameEvents.push_back (war_started);

        /// These civilizations are at war, they don't like each other.
        gameVariables.Civilizations[civilizationIndex].relationsWithOtherCivilizations[targetCivilizationIndex] -= 60;
        gameVariables.Civilizations[targetCivilizationIndex].relationsWithOtherCivilizations[civilizationIndex] -= 60;


    }

}

void Game::bringBelligerentAlliesIntoWar (War &war, int belligerentCivilizationIndex) {

    for (unsigned int i = 0; i < gameVariables.Civilizations.size (); i++) {

        if (civilizationsAreAllies (belligerentCivilizationIndex, i) && i != belligerentCivilizationIndex) {

            for (unsigned int j = 0; j < war.defenderCivilizationIndices.size (); j++) {

                if (war.defenderCivilizationIndices [j] == belligerentCivilizationIndex) {

                    war.defenderCivilizationIndices.push_back (i);

                }

            }

            for (unsigned int j = 0; j < war.offenderCivilizationIndices.size (); j++) {

                if (war.offenderCivilizationIndices [j] == belligerentCivilizationIndex) {

                    war.offenderCivilizationIndices.push_back (i);

                }

            }

        }

    }

}

bool Game::civilizationsAreAllies (int civilizationIndex, int otherCivilizationIndex) {

    for (unsigned int i = 0; i < gameVariables.alliances.size (); i++) {

        bool firstCivilizationIsInAlliance = false, secondCivilizationIsInAlliance = false;

        for (unsigned int j = 0; j < gameVariables.alliances [i].memberCivilizationIndices.size (); j++) {

            if (gameVariables.alliances [i].memberCivilizationIndices [j] == civilizationIndex) {

                firstCivilizationIsInAlliance = true;

            }

            if (gameVariables.alliances [i].memberCivilizationIndices [j] == otherCivilizationIndex) {

                secondCivilizationIsInAlliance = true;

            }

        }

        if (firstCivilizationIsInAlliance && secondCivilizationIsInAlliance) {

            return true;

        }

    }

    return false;

}
