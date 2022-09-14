#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "ksp.hpp"


using json = nlohmann::json;

using namespace std;


enum PartTypes {
	UNKNOWN,
	LFO_ENGINE,
	LFO_TANK,
	BOOSTER,
	DECOUPLER,
};

KSP::Engine convertEngine(json part) {
	KSP::Engine engine;

	engine.name = part["name"];
	engine.mass = part["mass"];

	engine.vacIsp = part["vacIsp"];
	engine.atmIsp = part["atmIsp"];

	engine.vacThrust = part["vacThrust"];
	engine.atmThrust = part["atmThrust"];

	return engine;
}

int main() {
	ifstream partsFile("parts.json");
	json partsJson;
	partsFile >> partsJson;

	vector<KSP::Engine> engines;

	for (auto& pack : partsJson["packs"]) {
		for (auto& part : pack["parts"]) {
			if (part["type"] == "TYPES.LFO_ENGINE") {
				engines.push_back(convertEngine(part));
			}
		}
	}


	ofstream file;

	file.open("partdata/engines.dat", ios::trunc);
	for (const auto& part : engines) { 
		file << part;
		cout << part.name << endl;
		cout << part.mass << endl;
		cout << part.vacIsp << endl;
		cout << part.atmIsp << endl;
		cout << part.vacThrust << endl;
		cout << part.atmThrust << endl;
	}
	file.close();

	return 0;
}