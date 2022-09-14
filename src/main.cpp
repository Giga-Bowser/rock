#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <numeric>
#include <execution>
#include <cmath>
#include <random>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "ksp.hpp"

using namespace std;
using namespace KSP;

random_device dev;
mt19937 rng(dev());
uniform_real_distribution<double> frand(0, 1);

const pair<const char*, double> planets[17] = {
	{"None", 0},
	{"Moho", 2.70},
	{"Eve", 16.7},
	{"Gilly", 0.049},
	{"Kerbin", 9.81},
	{"Mun", 1.63},
	{"Minmus", 0.491},
	{"Duna", 2.94},
	{"Ike", 1.10},
	{"Dres", 1.13},
	{"Jool", 7.85},
	{"Laythe", 7.85},
	{"Vall", 2.31},
	{"Tylo", 7.85},
	{"Bop", 0.589},
	{"Pol", 0.373},
	{"Eeloo", 1.69}
};

vector<Engine> engines;

const Engine nerv{ "LV-N \"Nerv\" Atomic Rocket Motor", 3.0, 800.0, 185.0, 60.00, 13.88 };
constexpr double nervRatio = 1.0 / 9.0;

constexpr double fuelRatio = 0.125;

Stage findOptimalStage(Args args) {

	double bestMass = __DBL_MAX__;

	Stage bestStage;

	for (const auto& engine : engines) {
		const double R = exp(args.deltaV / (engine.isp(args.atm) * 9.81));
		for (int i = 1; i <= 9; i++) { // FANCY rocket equation solving for fuel mass
			const double payload = engine.mass * i + args.payload;
			const double fuelMass = (R - 1) * payload / (fuelRatio + 1.0 - R * fuelRatio);
			const double totalMass = (fuelRatio + 1.0) * fuelMass + payload;

			if (i * engine.thrust(args.atm) / (totalMass * args.gravity) < args.twr) continue;

			if (totalMass < bestMass) {
				bestMass = totalMass;
				bestStage = Stage{ engine, i, totalMass };
			}

			break;
		}
	}

	// nerv time baby

	const double R = exp(args.deltaV / (nerv.isp(args.atm) * 9.81));
	for (int i = 1; i <= 9; i++) {
		const double payload = nerv.mass * i + args.payload;
		const double fuelMass = (R - 1) * payload / (nervRatio + 1.0 - R * nervRatio);
		const double totalMass = (nervRatio + 1.0) * fuelMass + payload;

		if (i * nerv.thrust(args.atm) / (totalMass * args.gravity) < args.twr) continue;

		if (totalMass < bestMass) {
			return Stage{ nerv, i, totalMass };
		}
	}


	return bestStage;
}

vector<Stage> findRandomMulti(MultiArgs args, double frac) {
	vector<Stage> solution;

	while (args.stageCount > 1) {
		Args firstArgs = args.toArgs();
		firstArgs.deltaV = args.deltaV * frac;

		solution.push_back(findOptimalStage(firstArgs));

		// prepare next args
		args.deltaV -= firstArgs.deltaV;
		args.atm.pop_back();
		args.twr.pop_back();
		args.payload = solution.back().mass;
		args.stageCount -= 1;
	}
	solution.push_back(findOptimalStage(args.toArgs()));

	return solution;
}

int main(int argc, char** argv) {
	ifstream engineFile("partdata/engines.dat");

	Engine temp;
	while (engineFile.peek() != EOF) {
		engineFile >> temp;
		engines.push_back(temp);
	}

	MultiArgs args{ 10.0, 3400.0, 9.81, 2, {1, 0.5}, {1.2, 0.8} };

	vector<Stage> best = {};


	const int maxIter = 1000;



	if (!glfwInit()) return 1;

	// GL 4.6 + GLSL 460
	const char* glsl_version = "#version 460";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(540, 720, "KSP Optimal Rockets Calculator++", NULL, NULL);
	if (window == NULL) return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Font
	ImFont* font = io.Fonts->AddFontFromFileTTF("JetBrainsMono.ttf", 18.0f);
	IM_ASSERT(font != NULL);


	// Main loop
	while (!glfwWindowShouldClose(window)) {

		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			static float f = 0.0f;
			static int counter = 0;
			
			ImGui::Begin("Arguments", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);


			ImGui::PushItemWidth(300);

			ImGui::InputDouble("Payload Mass, t", &args.payload, 1.0, 10.0, "%.2f");
			ImGui::InputDouble("Delta-v, m/s", &args.deltaV, 1.0, 10.0, "%.2f");
			if (ImGui::InputInt("Stage Count", &args.stageCount, 1, 0)) {
				args.atm.resize(args.stageCount);
				args.twr.resize(args.stageCount);
			}

			ImGui::InputDouble("Gravity, m/s^2", &args.gravity, 1.0, 10.0, "%.2f");
			static int selectedPlanet = 4;
			if (ImGui::BeginCombo("Planet(used for TWR)", planets[selectedPlanet].first)) {
				for (size_t i = 0; i < IM_ARRAYSIZE(planets); i++) {
					if (ImGui::Selectable(planets[i].first, false)) {
						selectedPlanet = i;
						args.gravity = planets[i].second;
					}
				}
				ImGui::EndCombo();
			}

			ImGui::PopItemWidth();

			ImGui::NewLine();
			ImGui::Text("Per stage settings:");

			ImGui::PushItemWidth(100);
			for (size_t i = 0; i < args.stageCount; i++) {
				ImGui::PushID(i);

				ImGui::Text("Stage %lu:", i + 1); ImGui::SameLine();
				ImGui::Text("Pressure, atm: "); ImGui::SameLine();
				ImGui::InputDouble("##ATM", &args.atm[args.atm.size() - i - 1], 0.1, 1, "%.2f"); ImGui::SameLine();
				ImGui::Text("TWR: "); ImGui::SameLine();
				ImGui::InputDouble("##TWR", &args.twr[args.twr.size() - i - 1], 0.1, 1, "%.2f");

				ImGui::PopID();
			}

			ImGui::PopItemWidth();


			ImGui::NewLine();
			ImGui::SetNextItemWidth(0);
			if (ImGui::Button("Generate!", ImVec2{ImGui::GetContentRegionAvail().x, 0})) {
				best = findRandomMulti(args, 0.5);

				vector<Stage> rocket;
				for (int i = 0; i < maxIter; i++) {
					rocket = findRandomMulti(args, i / (double)maxIter);
					
					if (rocket.back().mass < best.back().mass) 
						best = rocket;
				}
			}

			ImGui::End();


			ImGui::Begin("Results", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

			for (const auto& stage : best) {
				ImGui::Text("%s x %i: %.2ft", stage.engine.name.c_str(), stage.count, stage.mass);
			}

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}