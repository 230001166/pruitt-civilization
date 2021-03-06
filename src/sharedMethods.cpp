#include "sharedMethods.h"
#include "LoanEvent.h"
#include <math.h>
#include <iostream>
#include <thread>
#include <climits>
#include <random>


namespace sharedMethods {

int returnAcceptableIntegerInput () {

    int Input = 0;

    std::cin >> Input;

    while (!std::cin) {

        std::cout << "Bad value!";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> Input;

    }

    return Input;

}

int bindIntegerInputToRange (int minimumAcceptableValue, int maximumAcceptableValue, int defaultValue) {

    int integerInput = returnAcceptableIntegerInput();

    if (integerInput > maximumAcceptableValue || integerInput < minimumAcceptableValue) {

        integerInput = defaultValue;

    }

    return integerInput;

}

int getDistance(int x, int y, int x2, int y2) {

    int distance = sqrt(((x-x2) * (x-x2)) + ((y-y2) * (y-y2)));

    return distance;

}

bool isResearchComplete (int civilizationIndex, GameVariables &gameVariables) {

    if (gameVariables.Civilizations[civilizationIndex].researchPoints >=
        gameVariables.Civilizations[civilizationIndex].technologyBeingResearched.scienceCostToLearnResearch) {

        return true;

    } else {

        return false;

    }

}

int getUnitIndexByName (std::string name, GameVariables &gameVariables) {

    for (unsigned int i = 0; i < gameVariables.Units.size(); i++) {

        if (gameVariables.Units[i].name == name) {

            return i;

        }

    }

    std::cout << "Couldn't find unit " << name << "!";

    return 0;

}

bool unitIsNotTrespassing (int civilizationIndex, int xPositionToMoveTo, int yPositionToMoveTo, WorldMap worldMap) {

    if (worldMap.WorldTerritoryMap[xPositionToMoveTo][yPositionToMoveTo] == 0 ||
    worldMap.WorldTerritoryMap[xPositionToMoveTo][yPositionToMoveTo] == civilizationIndex+1) {

        return true;

    } else {

        return false;

    }

}

bool UnitisOnAnAncientRuin (Unit &unit, WorldMap &worldMap) {

    const int RUINS_CODE = 5;

    if (worldMap.featureMap[unit.position.x][unit.position.y] == RUINS_CODE) {

        return true;

    } else {

        return false;

    }

}

void getAncientRuinBenefits (Unit &unit, Civilization &civ, WorldMap &worldMap) {

    const int technologyBoost = 1, explorationBoost = 2;

    if (UnitisOnAnAncientRuin(unit, worldMap)) {

        int boostCode = rand () % 2 + 1;

        switch (boostCode) {

            case technologyBoost: {

                civ.researchPoints = civ.technologyBeingResearched.scienceCostToLearnResearch;

                worldMap.featureMap[unit.position.x][unit.position.y] = worldMap.mapTiles::GRASSLAND;

                std::cout << civ.CivName << " found a technology in an ancient ruin!" << std::endl;

            } break;

            case explorationBoost: {

                int x = rand() % 6 - 3;
                int y = rand() % 6 - 3;

                for (int i = -3; i < 4; i++) {

                    for (int j = -3; j < 4; j++) {

                        if (i+x+unit.position.x >= 0 && i+x+unit.position.x < worldMap.worldSize
                            && j+y+unit.position.y >= 0 && j+y+unit.position.y < worldMap.worldSize*4) {

                            civ.WorldExplorationMap[i+x+unit.position.x][j+y+unit.position.y] = 1;

                        }

                    }

                }

                std::cout << civ.CivName << " found a map of the surrounding territory in an ancient ruin!" << std::endl;

            } break;

        }

    }

}

void seizeTerritory (int civilizationIndex, int x, int y, GameVariables &gameVariables) {

    gameVariables.worldMap.WorldTerritoryMap[x][y] = civilizationIndex+1;

}

void civilizationTakeCity (int civilizationIndex, int cityIndex, GameVariables &gameVariables) {



    int cityX = gameVariables.Cities[cityIndex].position.x, cityY = gameVariables.Cities[cityIndex].position.y;

    for (int i = -3; i < 4; i++) {

        for (int j = -3; j < 4; j++) {

            if (sharedMethods::getDistance (cityX, cityY, cityX+i, cityY+j) < 2
                && gameVariables.worldMap.WorldTerritoryMap[cityX+i][cityY+j] == gameVariables.Cities[cityIndex].parentIndex+1) {

                gameVariables.worldMap.WorldTerritoryMap[cityX+i][cityY+j] = civilizationIndex+1;

            }

        }

    }

    gameVariables.Cities[cityIndex].parentIndex = civilizationIndex;

}

int getCityIndexAtPosition (int x, int y, GameVariables &gameVariables) {

    const int no_city_on_position = -1;

    for (unsigned int i = 0; i < gameVariables.Cities.size(); i++) {

        if (gameVariables.Cities[i].position.x == x && gameVariables.Cities[i].position.y == y) {

            return i;

        }

    }

    return no_city_on_position;

}

void moveUnit (Unit &unit, int deltaX, int deltaY, GameVariables &gameVariables) {

    bool isFlatTerrain = true;

    int xPositionToMoveTo = unit.position.x + deltaX, yPositionToMoveTo = unit.position.y + deltaY;

    if (unitCanMoveToTile (unit, xPositionToMoveTo, yPositionToMoveTo, gameVariables)) {

        if (gameVariables.worldMap.featureMap[xPositionToMoveTo][yPositionToMoveTo] == gameVariables.worldMap.mapTiles::MOUNTAIN
            || gameVariables.worldMap.featureMap[xPositionToMoveTo][yPositionToMoveTo] == gameVariables.worldMap.mapTiles::FOREST) {

            isFlatTerrain = false;
        }

        if (isFlatTerrain) { unit.movementPoints--; } else { unit.movementPoints -= unit.terrainMoveCost; }

        unit.position.setCoordinates(unit.position.x + deltaX, unit.position.y + deltaY);

        getAncientRuinBenefits (unit, gameVariables.Civilizations [unit.parentCivilizationIndex], gameVariables.worldMap);

        if (areCivsAtWar(unit.parentCivilizationIndex, gameVariables.worldMap.WorldTerritoryMap[xPositionToMoveTo][yPositionToMoveTo]-1, gameVariables)) {

            seizeTerritory (unit.parentCivilizationIndex, xPositionToMoveTo, yPositionToMoveTo, gameVariables);

            int cityIndex = getCityIndexAtPosition (unit.position.x, unit.position.y, gameVariables);

            if (cityIndex != -1) {

                civilizationTakeCity (unit.parentCivilizationIndex, cityIndex, gameVariables);

            }

        }
    }

    for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {

        for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {

            if (getDistance(i,j,unit.position.x,unit.position.y) <= 2) {
                gameVariables.Civilizations [unit.parentCivilizationIndex].WorldExplorationMap[i][j] = 1;
            }

        }
    }

}

bool unitCanMoveToTile (Unit &unit, int xPositionToMoveTo, int yPositionToMoveTo, GameVariables &gameVariables) {

    if (((unit.domain.canCoastalEmbark == false && gameVariables.worldMap.featureMap[xPositionToMoveTo][yPositionToMoveTo] != gameVariables.worldMap.mapTiles::COAST)
            || unit.domain.canCoastalEmbark)
        && ((unit.domain.canCrossOceans == false && gameVariables.worldMap.featureMap[xPositionToMoveTo][yPositionToMoveTo] != gameVariables.worldMap.mapTiles::OCEAN)
            || unit.domain.canCrossOceans)
        && (unitIsNotTrespassing(unit.parentCivilizationIndex, xPositionToMoveTo, yPositionToMoveTo, gameVariables.worldMap)
            ||  areCivsAtWar(unit.parentCivilizationIndex, gameVariables.worldMap.WorldTerritoryMap[xPositionToMoveTo][yPositionToMoveTo]-1,gameVariables))
        && enemyUnitIsNotOnTile (unit, xPositionToMoveTo, yPositionToMoveTo, gameVariables)) {

        return true;

    } else {

        return false;

    }

}

bool enemyUnitIsNotOnTile (Unit &unit, int xPositionToMoveTo, int yPositionToMoveTo, GameVariables &gameVariables) {

    for (unsigned int i = 0; i < gameVariables.UnitsInGame.size(); i++) {

        if (gameVariables.UnitsInGame[i].position.x == xPositionToMoveTo && gameVariables.UnitsInGame[i].position.y == yPositionToMoveTo) {

            if (areCivsAtWar(unit.parentCivilizationIndex, gameVariables.UnitsInGame[i].parentCivilizationIndex, gameVariables)) {

                return false;

            }

        }

    }

    return true;

}

Position returnValidPositionForUnitDeployment (Unit &unit, int cityIndex, int civilizationIndex, GameVariables &gameVariables) {

    Position cityPosition = gameVariables.Cities[cityIndex].position, temp;

    for (int i = -1; i < 2; i++) {

        for (int j = -1; j < 2; j++) {

            if (gameVariables.worldMap.featureMap[cityPosition.x + i][cityPosition.y + j] == gameVariables.worldMap.mapTiles::COAST
                && unit.domain.name != "Land") {

                temp.setCoordinates (cityPosition.x + i, cityPosition.y + j); return temp;

            } else if (gameVariables.worldMap.featureMap[cityPosition.x + i][cityPosition.y + j] != gameVariables.worldMap.mapTiles::COAST
                && unit.domain.name == "Land") {

                temp.setCoordinates (cityPosition.x + i, cityPosition.y + j); return temp;

            }

        }

    }

    temp.setCoordinates (-1, -1); return temp;


}

void deployUnit (Unit &unit, int cityIndex, int civilizationIndex, GameVariables &gameVariables) {

    Position temp_position = returnValidPositionForUnitDeployment (unit, cityIndex, civilizationIndex, gameVariables);

    if (temp_position.x != -1 && temp_position.y != -1) {

        unit.position.setCoordinates (temp_position.x, temp_position.y);

        unit.parentCivilizationIndex = civilizationIndex;

        gameVariables.UnitsInGame.push_back (unit);

    } else {

        std::cout << "[!] No valid tile to deploy unit " << unit.name << "! (@ deployUnit)" << std::endl;

    }

}

int getBuildingIndexByName (std::string name, GameVariables &gameVariables) {

    for (unsigned int i = 0; i < gameVariables.Buildings.size(); i++) {

        if (gameVariables.Buildings[i].name == name) {

            return i;

        }

    }

    std::cout << "Couldn't find building!";

    return 0;

}

int getResearchIndexByName (int civilizationIndex, std::string techName, GameVariables &gameVariables) {

    for (unsigned int i = 0; i < gameVariables.Civilizations[civilizationIndex].technologiesToResearch.size(); i++) {

        if (techName == gameVariables.Civilizations[civilizationIndex].technologiesToResearch[i].researchName) {

            return i;

        }

    }

    return -1;

}

bool CivilizationHasPrerequisiteTechs (int civilizationIndex, std::string techName, GameVariables &gameVariables) {

    int techPrequisitesMet = 0;

    int researchIndex = getResearchIndexByName (civilizationIndex, techName, gameVariables);

    if (researchIndex > -1) {

        for (unsigned int i = 0; i < gameVariables.Civilizations[civilizationIndex].technologiesToResearch[researchIndex].prerequisiteTechnologiesRequired.size(); i++) {

            for (unsigned int j = 0; j < gameVariables.Civilizations[civilizationIndex].learnedTechnologies.size(); j++) {

                if (gameVariables.Civilizations[civilizationIndex].technologiesToResearch[researchIndex].prerequisiteTechnologiesRequired[i] == gameVariables.Civilizations[civilizationIndex].learnedTechnologies[j]) {

                    techPrequisitesMet++;

                }

            }

        }

    }

    if (techPrequisitesMet == gameVariables.Civilizations[civilizationIndex].technologiesToResearch[researchIndex].prerequisiteTechnologiesRequired.size()) {
        return true;
    } else {
        return false;
    }

}

bool civilizationHasTechnology (int civilizationIndex, std::string techName, GameVariables &gameVariables) {

    for (unsigned int i = 0; i < gameVariables.Civilizations[civilizationIndex].learnedTechnologies.size(); i++) {

        if (gameVariables.Civilizations[civilizationIndex].learnedTechnologies[i] == techName) {

            return true;

        }

    }

    return false;

}

bool isTileBorderingCivilizationTerritory (int x, int y, int civilizationIndex, GameVariables gameVariables) {

    for (int i = -1; i < 2; i++) {

        for (int j = -1; j < 2; j++) {

            if (x+i >= 0 && x+i <= gameVariables.worldMap.worldSize && y+j >= 0 && y+j <= gameVariables.worldMap.worldSize*4) {

                if (gameVariables.worldMap.WorldTerritoryMap[i+x][j+y] == civilizationIndex+1 && sharedMethods::getDistance (x+i, y+j, x, y) <= 1) {

                    return true;

                }

            }

        }

    }

    return false;

}

int getTileFoodYield (int i, int j, GameVariables &gameVariables) {

    switch (gameVariables.worldMap.featureMap[i][j]) {

        case gameVariables.worldMap.mapTiles::OCEAN:
            return 1;
        break;

        case gameVariables.worldMap.mapTiles::COAST:
            return 1;
        break;

        case gameVariables.worldMap.mapTiles::GRASSLAND:
            return 2;
        break;

        case gameVariables.worldMap.mapTiles::FOREST:
            return 1;
        break;

        default:
            return 0;
        break;

    }

}

int getTileProductionYield (int i, int j, GameVariables &gameVariables) {

    switch (gameVariables.worldMap.featureMap[i][j]) {

        case gameVariables.worldMap.mapTiles::FOREST:
            return 1;
        break;

        case gameVariables.worldMap.mapTiles::MOUNTAIN:
            return 2;
        break;

        default:
            return 0;
        break;

    }

}

void foundCity (int x, int y, int civilizationIndex, GameVariables &gameVariables) {

    City city;

    city.position.x = x;
    city.position.y = y;

    int cityNameIndex = rand() % gameVariables.Civilizations[civilizationIndex].cityNames.size();

    city.cityName = gameVariables.Civilizations[civilizationIndex].cityNames[cityNameIndex];

    city.AvailableBuildingsToCreate.push_back("Granary");
    city.AvailableBuildingsToCreate.push_back("Library");
    city.AvailableBuildingsToCreate.push_back("Market");

    for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {
        for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {
            if (getDistance(i,j,x,y) <= 1) {
                gameVariables.worldMap.WorldTerritoryMap[i][j] = civilizationIndex+1;
            }
        }
    }

    city.parentIndex = civilizationIndex;

    gameVariables.Cities.push_back(city);

    assignWorkByPopulation(gameVariables.Cities.size() - 1, false, gameVariables);

}

void assignWorkByPopulation (int cityIndex, bool stopAfterNeededAmountIsCollected, GameVariables &gameVariables) {

    int assignedCitizens = 0;

    int highestFoodValueTiles[gameVariables.Cities[cityIndex].Population];

    int tileArraySize = (sizeof(highestFoodValueTiles)/sizeof(*highestFoodValueTiles));

    int bestFoodTileXPositions[gameVariables.Cities[cityIndex].Population];

    int bestFoodTileYPositions[gameVariables.Cities[cityIndex].Population];

    for (int a = 0; a < tileArraySize; a++) { highestFoodValueTiles[a] = 0; }

    gameVariables.Cities[cityIndex].FoodPerTurnFromTiles = 0;

    gameVariables.Cities[cityIndex].ProductionFromTiles = 0;

    for (int i = -4; i < 4; i++) {

        if (i + gameVariables.Cities[cityIndex].position.x < 0) {

            i++;

        }

        for (int j = -4; j < 4; j++) {

            if (j + gameVariables.Cities[cityIndex].position.y < 0) {

                j++;

            }

            if (i + gameVariables.Cities[cityIndex].position.x >= 0 && j + gameVariables.Cities[cityIndex].position.y >= 0) {

                int yield = getTileFoodYield (i + gameVariables.Cities[cityIndex].position.x, j + gameVariables.Cities[cityIndex].position.y, gameVariables);

                for (int k = 0; k < tileArraySize; k++) {

                    if (yield > highestFoodValueTiles[k] && gameVariables.worldMap.WorldTerritoryMap[i + gameVariables.Cities[cityIndex].position.x][j + gameVariables.Cities[cityIndex].position.y] == gameVariables.Cities[cityIndex].parentIndex+1) {

                        highestFoodValueTiles[k] = yield;
                        bestFoodTileXPositions[k] = i + gameVariables.Cities[cityIndex].position.x;
                        bestFoodTileYPositions[k] = j + gameVariables.Cities[cityIndex].position.y;
                        gameVariables.Cities[cityIndex].FoodPerTurnFromTiles += yield;
                        gameVariables.Cities[cityIndex].ProductionFromTiles += getTileProductionYield(i + gameVariables.Cities[cityIndex].position.x, j + gameVariables.Cities[cityIndex].position.y, gameVariables);
                        assignedCitizens++;

                        k = tileArraySize;

                        int foodIncrease = ((gameVariables.Cities[cityIndex].FoodPerTurnFromCity + gameVariables.Cities[cityIndex].FoodPerTurnFromTiles) - (gameVariables.Cities[cityIndex].Population*2));

                        if (foodIncrease > 0 && stopAfterNeededAmountIsCollected == true) {

                            i = 4;

                        }

                    }

                }

            }


        }

    }


}

int returnBaseUnitUpkeep (int civilizationIndex, GameVariables &gameVariables) {

    return gameVariables.Civilizations[civilizationIndex].era;

}

void checkIfCivilizationHasMetNewCivilization (int civilizationIndex, GameVariables &gameVariables) {

    for (int i = 0; i < gameVariables.worldMap.worldSize; i++) {

        for (int j = 0; j < gameVariables.worldMap.worldSize*4; j++) {

            if (gameVariables.Civilizations[civilizationIndex].WorldExplorationMap[i][j] == 1) {

                checkIfUnmetCivilizationUnitIsAtPosition (i, j, civilizationIndex, gameVariables);

                checkIfUnmetCivilizationOwnsPosition (i, j, civilizationIndex, gameVariables);

            }


        }

    }

}

void checkIfUnmetCivilizationUnitIsAtPosition (int x, int y, int civilizationIndex, GameVariables &gameVariables) {

    for (unsigned int k = 0; k < gameVariables.UnitsInGame.size(); k++) {

        if (gameVariables.UnitsInGame[k].position.x == x && gameVariables.UnitsInGame[k].position.y == y) {

            if (gameVariables.Civilizations[civilizationIndex].hasMetCivilization (gameVariables.UnitsInGame[k].parentCivilizationIndex) == false) {

                gameVariables.Civilizations[civilizationIndex].hasMetCivilizations[gameVariables.UnitsInGame[k].parentCivilizationIndex] = true;

                gameVariables.Civilizations[gameVariables.UnitsInGame[k].parentCivilizationIndex].hasMetCivilizations[civilizationIndex] = true;

            }

        }

    }


}

void checkIfUnmetCivilizationOwnsPosition (int x, int y, int civilizationIndex, GameVariables &gameVariables) {

    if (gameVariables.worldMap.WorldTerritoryMap[x][y] != civilizationIndex+1
        && gameVariables.Civilizations[civilizationIndex].hasMetCivilization (gameVariables.worldMap.WorldTerritoryMap[x][y]-1) == false) {

        if (gameVariables.worldMap.WorldTerritoryMap[x][y]-1 > -1) {

            gameVariables.Civilizations[civilizationIndex].hasMetCivilizations[gameVariables.worldMap.WorldTerritoryMap[x][y]-1] = true;

            gameVariables.Civilizations[gameVariables.worldMap.WorldTerritoryMap[x][y]-1].hasMetCivilizations[civilizationIndex] = true;

        }

    }

}

bool areCivsAtWar (int civilizationIndex, int targetCivilizationIndex, GameVariables &gameVariables) {

    int warIndex = returnIndexOfWarContainingCivilizations (civilizationIndex, targetCivilizationIndex, gameVariables);

    if (warIndex > -1) {

        bool civOneIsAttacking = false, civTwoIsAttacking = false;

         for (unsigned int j = 0; j < gameVariables.wars[warIndex].offenderCivilizationIndices.size(); j++) {

            if (gameVariables.wars[warIndex].offenderCivilizationIndices[j] == civilizationIndex) { civOneIsAttacking = true; }
            if (gameVariables.wars[warIndex].offenderCivilizationIndices[j] == targetCivilizationIndex) { civTwoIsAttacking = true; }

        }

        if ((civOneIsAttacking && !civTwoIsAttacking) || (!civOneIsAttacking && civTwoIsAttacking)) {

            return true;

        }

    } else {

        return false;

    }

    return false;

}

int returnIndexOfWarContainingCivilizations (int civilizationIndex, int targetCivilizationIndex, GameVariables &gameVariables) {

    bool containsCivOne = false, containsCivTwo = false;

    for (unsigned int i = 0; i < gameVariables.wars.size(); i++) {

        containsCivOne = false; containsCivTwo = false;

        for (unsigned int j = 0; j < gameVariables.wars[i].offenderCivilizationIndices.size(); j++) {

            if (gameVariables.wars[i].offenderCivilizationIndices[j] == civilizationIndex) { containsCivOne = true; }
            if (gameVariables.wars[i].offenderCivilizationIndices[j] == targetCivilizationIndex) { containsCivTwo = true; }

        }

        for (unsigned int j = 0; j < gameVariables.wars[i].defenderCivilizationIndices.size(); j++) {

            if (gameVariables.wars[i].defenderCivilizationIndices[j] == civilizationIndex) { containsCivOne = true; }
            if (gameVariables.wars[i].defenderCivilizationIndices[j] == targetCivilizationIndex) { containsCivTwo = true; }

        }

        if (containsCivOne && containsCivTwo) { return i; }

    }

    return -1;

}

int returnNumberOfCitiesCivilizationOwns (int civilizationIndex, GameVariables &gameVariables) {

    int numberOfCitiesOwnedByCivilization = 0;

    for (unsigned int i = 0; i < gameVariables.Cities.size (); i++) {

        if (gameVariables.Cities [i].parentIndex == civilizationIndex) {

            numberOfCitiesOwnedByCivilization++;

        }

    }

    return numberOfCitiesOwnedByCivilization;

}

void createNewLoanEventNotification (int civilizationIndex, GameVariables &gameVariables, Loan temp_loan) {

    LoanEvent *loanEvent = new LoanEvent;

    loanEvent->LoanID = gameVariables.activeLoans.size() - 1;

    loanEvent->EventName = "Loan Request from " + gameVariables.Civilizations[civilizationIndex].CivName;
    loanEvent->EventMessage = "A foreign civilization has offered to loan us " + std::to_string(temp_loan.amountDue) + " gold.";

    loanEvent->ResponseChoices.push_back ("We'll take all the help we can get.");
    loanEvent->ResponseChoices.push_back ("We do not need help from lesser civilizations.");

    loanEvent->targetCivilizationIndex = temp_loan.debtorCivilizationIndex;

    gameVariables.gameEvents.push_back (loanEvent);

}

}

