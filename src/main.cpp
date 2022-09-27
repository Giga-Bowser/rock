#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "ksp.hpp"
#include "font.hpp"

using namespace std;
using namespace KSP;

const pair<const char*, double> planets[3] = {
	{"None", 0},
	{"Earth", 9.81},
	{"Moon", 1.63},
};

vector<Engine> engines;
vector<Engine> selectedEngines;

double fuelRatio = 0.05;

Stage findOptimalStage(Args args) {
	Stage bestStage{.mass = __DBL_MAX__};

	for (const auto& engine : selectedEngines) {
		const double R = exp(args.deltaV / (engine.isp(args.atm) * 9.81));
		for (int i = 1; i <= 9; i++) { // FANCY rocket equation solving for fuel mass
			const double payload = engine.mass * i + args.payload;
			const double fuelMass = (R - 1) * payload / (fuelRatio + 1.0 - R * fuelRatio);
			const double totalMass = (fuelRatio + 1.0) * fuelMass + payload;

			if (i * engine.thrust(args.atm) / (totalMass * args.gravity) < args.twr) continue;
			if (fuelMass / (i * engine.consumption()) > engine.burnTime) continue;

			if (totalMass < bestStage.mass) {
				bestStage = Stage{engine, i, totalMass};
			}

			break;
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

int main() {
	ifstream engineFile("partdata/roengines.dat");

	while (engineFile.peek() != EOF) {
		Engine temp;
		engineFile >> temp;
		engines.push_back(temp);
	}

	sort(engines.begin(), engines.end(), [](Engine& a, Engine& b) { return a.name < b.name; });

	vector<char> engineBools(engines.size(), false);

	MultiArgs args{10.0, 9400.0, 9.81, 2, {1, 0.1}, {1.2, 0.8}};

	vector<Stage> best = {};

	if (!glfwInit()) return 1;

	// GL 4.6 + GLSL 460
	const char* glsl_version = "#version 460";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);		   // 3.0+ only

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(540, 720, "Rocket Optimizing Calculator for C++", nullptr, nullptr);
	if (window == nullptr) return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Font
	ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(JetBrainsMono_compressed_data, JetBrainsMono_compressed_size, 18);
	IM_ASSERT(font != nullptr);

	// Main loop
	while (!glfwWindowShouldClose(window)) {

		// Poll and handle events (inputs, window resize, etc.)
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Arguments", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

			ImGui::PushItemWidth(300);

			ImGui::InputDouble("Payload Mass, t", &args.payload, 1.0, 10.0, "%.2f");
			ImGui::InputDouble("Delta-v, m/s", &args.deltaV, 1.0, 10.0, "%.2f");
			ImGui::InputDouble("Fuel ratio", &fuelRatio, 0.01, 0.1, "%.2f");

			if (ImGui::InputInt("Stage Count", &args.stageCount, 1, 0)) {
				args.atm.resize(args.stageCount);
				args.twr.resize(args.stageCount);
			}

			static int selectedPlanet = 1;
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
			for (int i = 0; i < args.stageCount; i++) {
				ImGui::PushID(i);

				ImGui::Text("Stage %i:", i + 1);
				ImGui::SameLine();
				ImGui::Text("Pressure, atm: ");
				ImGui::SameLine();
				ImGui::InputDouble("##ATM", &args.atm[args.atm.size() - i - 1], 0.1, 1, "%.2f");
				ImGui::SameLine();
				ImGui::Text("TWR: ");
				ImGui::SameLine();
				ImGui::InputDouble("##TWR", &args.twr[args.twr.size() - i - 1], 0.1, 1, "%.2f");

				ImGui::PopID();
			}
			ImGui::PopItemWidth();

			static bool allEngines;
			if (ImGui::Checkbox("Enable all engines?", &allEngines)) {
				fill(engineBools.begin(), engineBools.end(), allEngines);
			}

			if (ImGui::CollapsingHeader("Engines")) {
				for (size_t i = 0; i < engines.size(); i++) {
					ImGui::Checkbox(engines[i].name.c_str(), (bool*)&engineBools[i]);
				}

				ImGui::NewLine();

				static char saveName[128] = "save.dat";
				ImGui::InputText("Filename", saveName, IM_ARRAYSIZE(saveName));
				if (ImGui::Button("Save Selection", ImVec2{ImGui::GetContentRegionAvail().x / 2, 0})) {
					ofstream saveFile(saveName, ios::trunc | ios::binary);
					for (const auto& b : engineBools)
						saveFile << b;
					saveFile.flush();
				}
				ImGui::SameLine();
				if (ImGui::Button("Load Selection", ImVec2{ImGui::GetContentRegionAvail().x, 0})) {
					ifstream saveFile(saveName);
					for (auto& b : engineBools)
						saveFile >> b;
				}
			}

			ImGui::NewLine();
			if (ImGui::Button("Generate!", ImVec2{ImGui::GetContentRegionAvail().x, 0})) {
				selectedEngines = {};
				for (size_t i = 0; i < engineBools.size(); i++) {
					if (engineBools[i]) {
						selectedEngines.push_back(engines[i]);
					}
				}

				best = findRandomMulti(args, 0.5);

				vector<Stage> rocket;
				static const int maxIter = 1000;
				for (int i = 0; i < maxIter; i++) {
					rocket = findRandomMulti(args, (i / (double)maxIter));

					if (rocket.back().mass < best.back().mass) best = rocket;
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
