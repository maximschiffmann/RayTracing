/*
 * 	Defindes the user-interaction via shell prompt and script file.
 * 	
 * 	Would be nice to extend with readline capabilities.
 *
 */
#include "interaction.h"
#include "cmdline.h"

#include "libgi/timer.h"
#include "libgi/scene.h"
#include "libgi/algorithm.h"
#include "libgi/framebuffer.h"
#include "libgi/context.h"
#include "libgi/wavefront-rt.h"

#include "rt/seq/seq.h"
#include "rt/bbvh-base/bvh.h"
#include "gi/primary-hit.h"

#include "libgi/timer.h"

#include "libgi/global-context.h"

#include <iostream>
#include <sstream>

#include <glm/glm.hpp>
#if GLM_VERSION < 997
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

inline std::istream& operator>>(std::istream &in, vec3 &x) {
	in >> x.x >> x.y >> x.z;
	return in;
}

using namespace glm;
using namespace std;

const char *prompt = "rtgi > ";

#define ifcmd(c) if (command==c)
#define error_no_continue(x) { cerr << "command " << uc.cmdid << " (" << command << "): " << x << endl; }
#define error(x) { error_no_continue(x);  continue; }
#define check_in(x) { if (in.bad() || in.fail()) error(x); }
#define check_in_complete(x) { if (in.bad() || in.fail() || !in.eof()) error(x); }

void run(gi_algorithm *algo);
void rt_bench();

static bool align_rt_and_algo(scene &scene, gi_algorithm *algo, repl_update_checks &uc, const std::string &command) {
	if (scene.single_rt && dynamic_cast<wavefront_algorithm*>(algo)) {
		cout << "Wavefront algorithm used with invidividually tracing RT, promoting RT to batch tracing." << endl;
		auto *rt = scene.single_rt;
		scene.release_rt();
		scene.use(new wf::cpu::batch_rt_adapter(rt));
	}
	else if (scene.batch_rt && dynamic_cast<recursive_algorithm*>(algo)) {
		error_no_continue("Cannot drive a recursive algorithm by a batch ray tracer");
		return false;
	}
	return true;
}

void repl(istream &infile, repl_update_checks &uc) {
	bool cam_has_pos = false,
		 cam_has_dir = false,
		 cam_has_up = false,
		 scene_up_set = false;

	gi_algorithm *&algo = rc->algo;
	scene &scene = rc->scene;
	framebuffer &framebuffer = rc->framebuffer;

	material *mat = nullptr;
	vector<string> commands;

	while (!infile.eof()) {
		if (&infile == &cin)
			cout << prompt << flush;
		uc.cmdid++;
		string line, command;
		getline(infile, line);
		commands.push_back(line);
		istringstream in(line);

		in >> command;
		vec3 tmp;
		ifcmd("history") {
			commands.pop_back();
			for (auto &x : commands)
				cout << x << endl;
		}
		else ifcmd("quit")
			return;
		else ifcmd("exit")
			return;
		else ifcmd("at") {
			in.get();
			if (in.eof()) {
				cout << scene.camera.pos << endl;
				continue;
			}
			in >> tmp;
			check_in_complete("Syntax error, requires 3 numerical components");
			scene.camera.pos = tmp;
			cam_has_pos = true;
		}
		else ifcmd("look") {
			in.get();
			if (in.eof()) {
				cout << scene.camera.dir << endl;
				continue;
			}
			in >> tmp;
			check_in_complete("Syntax error, requires 3 numerical components");
			scene.camera.dir = tmp;
			cam_has_dir = true;
		}
		else ifcmd("up") {
			in.get();
			if (in.eof() && scene_up_set) {
				cout << scene.up << endl;
				continue;
			}
			if (scene_up_set)
				error("Cannot set scene up vector twice, did you mean camup?");
			in >> tmp;
			check_in_complete("Syntax error, requires 3 numerical components");
			scene_up_set = true;
			scene.up = tmp;
			if (!cam_has_up) {
				scene.camera.up = scene.up;
				cam_has_up = true;
			}
		}
		else ifcmd("camup") {
			in.get();
			if (in.eof()) {
				cout << scene.camera.up << endl;
				continue;
			}
			in >> tmp;
			check_in_complete("Syntax error, requires 3 numerical components");
			scene.camera.up = tmp;
			cam_has_up = true;
		}
		else ifcmd("load") {
			string file, name;
			in >> file;
			if (!in.eof())
				in >> name;
			check_in_complete("Syntax error, requires a file name (no spaces, sorry) and (optionally) a name");
			scene.add(file, name);
			uc.scene_touched_at = uc.cmdid;
		}
		else ifcmd("resolution") {
			int w, h;
			in >> w >> h;
			check_in_complete("Syntax error, requires 2 integral values");
			rc->change_resolution(w, h);
		}
		else ifcmd("algo") {
			string name;
			in >> name;
			gi_algorithm *a = nullptr;
			if (name == "primary")      a = new primary_hit_display;
			else if (name == "primary-wf")  a = new primary_hit_display_wf;
			else error("There is no gi algorithm called '" << name << "'");
			if (a) {
				delete algo;
				algo = a;
			}
			if (!align_rt_and_algo(scene, algo, uc, command)) continue;
		}
		else ifcmd("outfile") {
			string name;
			in >> name;
			check_in_complete("Syntax error, only accepts a single file name (no spaces, sorry)");
			cmdline.outfile = name;
		}
// 		else ifcmd("bookmark") {
// 			
// 		}
		else ifcmd("raytracer") {
			string name;
			in >> name;
			if (name == "seq") scene.use(new seq_tri_is);
			else if (name == "naive-bvh") scene.use(new naive_bvh);
			else if (name == "bbvh") {
				string tag1, tag2;
				in >> tag1 >> tag2;
				bool flat = true;
				bool esc = false;
				if (tag1 == "indexed" || tag2 == "indexed") flat = false;
				if (tag1 == "esc" || tag2 == "esc") esc = true;
				if (flat && !esc)
					scene.use(new binary_bvh_tracer<bbvh_triangle_layout::flat, bbvh_esc_mode::off>);
				else if (!flat && !esc)
					scene.use(new binary_bvh_tracer<bbvh_triangle_layout::indexed, bbvh_esc_mode::off>);
				else if (!flat && esc)
					scene.use(new binary_bvh_tracer<bbvh_triangle_layout::indexed, bbvh_esc_mode::on>);
				else if (flat && esc)
					error("This combination is technically problematic")
				else
					error("There is no such bbvh variant");
				if (!align_rt_and_algo(scene, algo, uc, command)) continue;
			}
			else error("There is no ray tracer called '" << name << "'");
			uc.tracer_touched_at = uc.cmdid;
		}
		else ifcmd("commit") {
			if (scene.vertices.empty())
				error("There is no scene data to work with");
			if (!scene.rt)
				error("There is no ray traversal scheme to commit the scene data to");
			if (!align_rt_and_algo(scene, algo, uc, command)) continue;
			scene.rt->build(&scene);
			uc.accel_touched_at = uc.cmdid;
		}
		else ifcmd("sppx") {
			int sppx;
			in >> sppx;
			check_in_complete("Syntax error, requires exactly one positive integral value");
			rc->sppx = sppx;
		}
		else ifcmd("run") {
			if (uc.scene_touched_at == 0 || uc.tracer_touched_at == 0 || uc.accel_touched_at == 0 || algo == nullptr)
				error("We have to have a scene loaded, a ray tracer set, an acceleration structure built and an algorithm set prior to running");
			if (uc.accel_touched_at < uc.tracer_touched_at)
				error("The current tracer does (might?) not have an up-to-date acceleration structure");
			if (uc.accel_touched_at < uc.scene_touched_at)
				error("The current acceleration structure is out-dated");
			run(algo);
		}
		else ifcmd("rt_bench") {
#ifndef WITH_STATS		
			if (uc.scene_touched_at == 0 || uc.tracer_touched_at == 0 || uc.accel_touched_at == 0)
				error("We have to have a scene loaded, a ray tracer set, an acceleration structure built prior to running");
			if (uc.accel_touched_at < uc.tracer_touched_at)
				error("The current tracer does (might?) not have an up-to-date acceleration structure");
			if (uc.accel_touched_at < uc.scene_touched_at)
				error("The current acceleration structure is out-dated");
			rt_bench(rc);
#else
			cerr << "ERROR: cannot run rt-bench when WITH_STATS is defined" << endl;
#endif
		}
		else ifcmd("mesh") {
			string name, cmd;
			in >> name;
			if (in.eof() && name == "list") {
				for (auto &obj : scene.objects) cout << obj.name << endl;
				continue;
			}
			error("Meshes can only be listed in this version");
		}
		else ifcmd("material") {
			string cmd;
			in >> cmd;
			if (in.eof() && cmd == "list") {
				for (auto &mtl : scene.materials) cout << mtl.name << endl;
				continue;
			}
			check_in("Syntax error, requires material name, command and subsequent arguments");
			command = cmd;
			ifcmd("select") {
				string name;
				in >> name;
				check_in_complete("Only a single string (no whitespace) accepted");
				material *m = nullptr; for (auto &mtl : scene.materials) if (mtl.name == name) { m = &mtl; break; }
				if (!m) error("No material called '" << name << "'");
				mat = m;
				continue;
			}
			if (!mat)
				error("No material selected");
			ifcmd("albedo") {
				in >> tmp;
				check_in_complete("Expects a color triplet");
				mat->albedo = tmp;
			}
			else ifcmd("emissive") {
				in >> tmp;
				check_in_complete("Expects a color triplet");
				mat->emissive = tmp;
			}
			else ifcmd("roughness") {
				in >> tmp.x;
				check_in_complete("Expects a floating point value");
				mat->roughness = tmp.x;
			}
			else ifcmd("ior") {
				in >> tmp.x;
				check_in_complete("Expects a floating point value");
				mat->ior = tmp.x;
			}
			else ifcmd("texture") {
				in >> cmd;
				check_in_complete("Expected a single (no whitespace) string value");
				// we keep the textures around as other materials might still use them.
				// they will be cleaned up by ~scene
				if (cmd == "drop")
					mat->albedo_tex = nullptr;
				else {
					texture *tex = load_image3f(cmd);
					if (tex)
						mat->albedo_tex = tex;
				}
			}
			else ifcmd("show") {
				check_in_complete("Does not take further arguments");
				cout << "albedo     " << mat->albedo << endl;
				cout << "albedo-tex " << (mat->albedo_tex ? mat->albedo_tex->path.string() : string("none")) << endl;
				cout << "emissive   " << mat->emissive << endl;
				cout << "roughness  " << mat->roughness << endl;
				cout << "ior        " << mat->ior << endl;
			}
			else error("Unknown subcommand");
		}
		else ifcmd("omp") {
			string sub;
			in >> sub;
			if (sub == "off")
				omp_set_num_threads(1);
			else if (sub == "on")
				omp_set_num_threads(omp_get_max_threads());
			else
				error("Syntax error: expected 'on' or 'off'");
		}
		else ifcmd("stats") {
			string sub;
			in >> sub;
			if (sub == "clear")
				stats_timer.clear();
			else if (sub == "print")
				stats_timer.print();
			else
				error("No such stats subcommand");
		}
		else ifcmd("echo") {
			string text;
			char c; in >> c; // skip first whitespace
			getline(in, text);
			cout << text << endl;
		}
		else if (command == "") ;
		else if (command[0] == '#') ;
		else if (algo && algo->interprete(command, in)) ;
		else if (scene.rt && scene.rt->interprete(command, in)) ;
		else {
			error("Unknown command");
		}
	}
	cout << endl;
}
