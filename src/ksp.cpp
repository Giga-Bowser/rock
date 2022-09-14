#include <numeric>
#include <cmath>
#include <iostream>

#include "ksp.hpp"

#define reduceProp(vec, t1, prop, t2)	accumulate(vec.cbegin(), vec.cend(), (t2)0, [](const t2& a, const t1& b) { return a + b.prop; })

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

KSP::Args KSP::MultiArgs::toArgs(int i) {
	return KSP::Args{payload, deltaV, gravity, atm[atm.size() - 1 - i], twr[twr.size() - 1 - i]};
}