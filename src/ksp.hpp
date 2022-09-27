#include <string>
#include <vector>

using namespace std;

namespace KSP {
	struct Engine {
		string name;
		double mass;

		double vacIsp;
		double atmIsp;

		double vacThrust;

		double burnTime;

		constexpr double isp(double atm) const { return vacIsp + atm * (atmIsp - vacIsp); };

		constexpr double thrust(double atm) const { return vacThrust * isp(atm) / vacIsp; };

		constexpr double consumption() const { return vacThrust / (vacIsp * 9.81); };
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

		Args toArgs();
	};

	struct Stage {
		Engine engine;
		int count;

		double mass;
	};
}; // namespace KSP

ostream& operator<<(ostream& os, const KSP::Engine& engine);
istream& operator>>(istream& is, KSP::Engine& engine);