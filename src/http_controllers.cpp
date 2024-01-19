#include <drogon/drogon.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "common_types.hpp"
#include "config.hpp"
#include "config_path.hpp"
#include "log.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

using namespace std;

using namespace drogon;

typedef std::function<void(const HttpResponsePtr &)> Callback;

void read_config_for_web()
{
	read_config();
}

bool jsonParse(std::string_view str, Json::Value& val, std::string& err)
{
  Json::CharReaderBuilder builder;
  Json::CharReader* reader = builder.newCharReader();
  val.clear();
  bool parsed = reader->parse(
    str.data(),
    str.data() + str.length(),
    &val,
    &err
  );
  delete reader;
  return parsed;
}

void fillJsonResponse(Json::Value& ret, HttpResponsePtr& resp)
{
	Json::FastWriter fastWriter;
	std::string json_str = fastWriter.write(ret);
	//
	resp->addHeader("Content-Type", "application/json");
	resp->addHeader("Access-Control-Allow-Origin", "*");
	resp->addHeader("Access-Control-Request-Method", "*");
	//
	resp->setBody(json_str);
}

void get_params(const HttpRequestPtr& request, Callback&& callback)
{
	write_log("get_params request!");
	//
	init_shared_memory();
	//
	Json::Value ret;
	ConfigData buf = *config_sm_ptr;
	//
	fill_json_form_config(buf, ret);
	//
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	fillJsonResponse(ret, resp);
	callback(resp);
}

void apply_params(const HttpRequestPtr &request, Callback &&callback)
{
	write_log("apply_params request!");
	//
	ConfigData buf;
	//
	Json::Value v;
	string err;
	string str;
	if(jsonParse(request->body(), v, err)) {
		try {
			read_config_sm(buf);
			//
			fill_config_form_json(v, buf);
			//
			write_config_sm(buf);
		} catch (...) {
			err = "apply params failed!";
			write_log(err);
		}
	} else
		write_log("jsonParse failed! " + err);
	//
	kill(config_sm_ptr->PID, SIGUSR1);
	//
	Json::Value ret;
	if (err == "")
		ret["message"] = "ok";
	else
		ret["message"] = "error";
	//
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	fillJsonResponse(ret, resp);
	callback(resp);
}

void save_params(const HttpRequestPtr &request, Callback &&callback)
{
	write_log("save_params request!");
	//
	Json::Value ret;
	string err;
	try {
		ConfigData buf = *config_sm_ptr;
		save_config(buf);
	} catch (...) {
		err = "save params failed!";
		write_log(err);
	}
	if (err == "")
		ret["message"] = "ok";
	else
		ret["message"] = "error";
	//
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	fillJsonResponse(ret, resp);
	callback(resp);
}

void fillParseResultJson(Json::Value& aJS, ResultFixed& parse_result)
{
	aJS.clear();
	aJS["width"] = parse_result.img_width;
	aJS["height"] = parse_result.img_height;
	aJS["error_flags"] = parse_result.error_flags;
	//
	Json::Value res_pt, res_pts;
	for (int16_t i = 0; i < parse_result.points_count; i++)
	{
		res_pt["x"] = parse_result.points[i].x;
		res_pt["y"] = parse_result.points[i].y;
		res_pts.append(res_pt);
	}
	aJS["res_points"] = res_pts;
	//
	aJS["center_x"] = parse_result.center_x;
	aJS["center_x_mm"] = parse_result.center_x_mm;
	//
	Json::Value hor_ys;
	for (int16_t i = 0; i < parse_result.hor_count; i++)
	{
		hor_ys.append(parse_result.points_hor[i]);
	}
	aJS["hor_ys"] = hor_ys;
	//
	aJS["zone_flags"] = parse_result.zone_flags;
	aJS["stop_distance"] = parse_result.stop_distance;
}

void get_points(const HttpRequestPtr &request, Callback &&callback)
{
	//write_log("get_points request!");
	//
	Json::Value root;
	Json::Value child;
	ResultFixed rfx;
	for (int i = 0; i < CAM_COUNT; i++) {
		read_results_sm(rfx, i);
		fillParseResultJson(child, rfx);
		root["result"].append(child);
	}
	//
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	fillJsonResponse(root, resp);
	callback(resp);
}

void get_config_map(const HttpRequestPtr &request, Callback &&callback)
{
	write_log("get_config_map request!");
	//
	Json::Value root;
	Json::Value child;
	string nm;
	for (const auto& element : config_map) {
		nm = element.first;
		ConfigItem ci = config_map[nm];
		Json::Value child;
		child["name"] = nm;
		child["type"] = (int)ci.type;
		child["descr"] = ci.description;
		root["result"].append(child);
	}
	//
	HttpResponsePtr resp = HttpResponse::newHttpResponse();
	fillJsonResponse(root, resp);
	callback(resp);
}

void http_init()
{
	string html_document_root = get_work_directory() + "html";
	write_log("html_document_root = " + html_document_root);
	//
	drogon::app().setLogLevel(trantor::Logger::LogLevel::kDebug);
	drogon::app().setThreadNum(1);
	drogon::app().enableServerHeader(false);
	drogon::app().setDocumentRoot(html_document_root);
	drogon::app().addListener("0.0.0.0", 8000);
	//
	drogon::app().registerHandler("/get_params", &get_params, { Get, Post, Options });
	drogon::app().registerHandler("/apply_params", &apply_params, { Get, Post, Options });
	drogon::app().registerHandler("/save_params", &save_params, { Get, Post, Options });
	drogon::app().registerHandler("/get_points", &get_points, { Get, Post, Options });
	drogon::app().registerHandler("/get_config_map", &get_config_map, { Get, Post, Options });
	//
	drogon::app().run();
}
