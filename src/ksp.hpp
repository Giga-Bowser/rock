#include <string>
#include <vector>
#include <cmath>

using namespace std;

namespace KSP {
	struct Engine {
		string name;
		double mass;

		double vacIsp;
		double atmIsp;

		double vacThrust;
		double atmThrust;

		constexpr double thrust(double atm) const { return lerp(vacThrust, atmThrust, atm); };

		constexpr double isp(double atm) const { return lerp(vacIsp, atmIsp, atm); };

		constexpr double consumption(double atm) const { return thrust(atm) / (isp(atm) * 9.81); };
	};

	struct Args {
		double payload;
		double deltaV;
		double gravity;

		double atm;
		double twr;
	};

	struct MultiArgs {
		double payload;
		double deltaV;
		double gravity;

		int stageCount;

		vector<double> atm;
		vector<double> twr;

		Args toArgs(int i = 0);
	};

	struct Stage {
		Engine engine;
		int count;

		double mass;
	};
};

ostream& operator<<(ostream& os, const KSP::Engine& engine);
istream& operator>>(istream& is, KSP::Engine& engine);