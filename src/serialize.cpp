#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "ksp.hpp"

using namespace std;
using namespace KSP;

vector<Engine> serializeEngine(string partString) {
	vector<Engine> engines;

	partString = regex_replace(partString, regex("\\s*\\/\\/.+"), "");

	smatch massMatch;
	regex_search(partString, massMatch, regex("origMass = (.+)"));
	double origMass = atof(massMatch[1].str().c_str());

	regex bodyRegex("\\tCONFIG\\n\\t*\\{((.|\\n)*?})\\s*}");
	auto bodyIt = sregex_iterator(partString.begin(), partString.end(), bodyRegex);

	for (; bodyIt != sregex_iterator(); ++bodyIt) {
		Engine engine;

		string bodyString = smatch(*bodyIt)[1].str();
		smatch name;
		regex_search(bodyString, name, regex("name = (.+)"));
		engine.name = name[1].str();

		smatch isp;
		regex_search(bodyString, isp, regex("key = 0 (.+)\\s+key = 1 (.+)"));
		engine.vacIsp = atof(isp[1].str().c_str());
		engine.atmIsp = atof(isp[2].str().c_str());

		smatch thrust;
		regex_search(bodyString, thrust, regex("maxThrust = (.+)"));
		engine.vacThrust = atof(thrust[1].str().c_str());

		smatch massMult;
		if (regex_search(bodyString, massMult, regex("massMult = (.+)"))) {
			engine.mass = atof(massMult[1].str().c_str()) * origMass;
		} else {
			engine.mass = origMass;
		}

		engines.push_back(engine);
	}

	regex testRegex("TESTFLIGHT\\s*\\{((.|\\s)*?)}");

	auto testIt = sregex_iterator(partString.begin(), partString.end(), testRegex);

	for (; testIt != sregex_iterator(); ++testIt) {
		string testString = smatch(*testIt)[1].str();

		smatch burnTime;
		regex_search(testString, burnTime, regex("\\s*name = (.+)\\n(.|\\s)*?ratedBurnTime = (\\d*.?\\d*)"));

		for (auto& engine : engines) {

			if (burnTime[1].str() == engine.name) {
				engine.burnTime = atof(burnTime[3].str().c_str());
			}
		}
	}

	return engines;
}

int main() {
	filesystem::directory_iterator engineIterator("Engine_Configs/");

	vector<Engine> engines;

	for (const auto& engineFile : engineIterator) {
		std::ifstream file(engineFile.path(), ios::binary | ios::ate);
		auto fileSize = file.tellg();
		file.seekg(ios::beg);

		string partString(fileSize, 0);
		file.read(&partString[0], fileSize);

		auto fileEngines = serializeEngine(partString);

		engines.insert(engines.cend(), fileEngines.cbegin(), fileEngines.cend());
	}

	ofstream outFile("partdata/roengines.dat", ios::trunc | ios::binary);
	for (const auto& engine : engines) {
		outFile << engine;
	}

	return 0;
}