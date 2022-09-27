#include <cmath>
#include <iostream>
#include <numeric>

#include "ksp.hpp"

using namespace std;

// Engine

ostream& operator<<(ostream& os, const KSP::Engine& engine) {
	os.write(engine.name.c_str(), engine.name.size() + 1);
	os.write((char*)&engine.mass, sizeof(engine) - sizeof(engine.name));

	return os;
}

istream& operator>>(istream& is, KSP::Engine& engine) {
	getline(is, engine.name, '\0'); // read null terminated string
	is.read((char*)&engine.mass, sizeof(engine) - sizeof(engine.name));

	return is;
}

// Args

KSP::Args KSP::MultiArgs::toArgs() {
	return KSP::Args{payload, deltaV, gravity, atm.back(), twr.back()};
}