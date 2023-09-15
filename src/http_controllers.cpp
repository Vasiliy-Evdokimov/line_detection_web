#include <iostream>

#include <drogon/drogon.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "defines.hpp"
#include "common_types.hpp"
#include "config.hpp"
#include "shared_memory.hpp"
#include "http_controllers.hpp"

using namespace std;

using namespace drogon;

typedef std::function<void(const HttpResponsePtr &)> Callback;

const string html_document_root =
	#ifndef RELEASE
		"/home/vevdokimov/eclipse-workspace/line_detection/html";
	#else
		"/home/user/line_detection/html";
	#endif

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
	std::cout << "get_params request!" << std::endl;
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
	std::cout << "apply_params request!" << std::endl;
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
			cout << err << endl;
		}
	} else
		cout << "jsonParse failed! " << err << endl;
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
	std::cout << "save_params request!" << std::endl;
	//
	Json::Value ret;
	string err;
	try {
		ConfigData buf = *config_sm_ptr;
		save_config(buf);
	} catch (...) {
		err = "save params failed!";
		cout << err << endl;
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
	aJS["fl_error"] = parse_result.error_flag;
	//
	Json::Value res_pt, res_pts;
	for (size_t i = 0; i < parse_result.points_count; i++) {
		res_pt["x"] = parse_result.points[i].x;
		res_pt["y"] = parse_result.points[i].y;
		res_pts.append(res_pt);
	}
	aJS["res_points"] = res_pts;
	//
	Json::Value hor_ys;
	for (size_t i = 0; i < parse_result.hor_count; i++)
		hor_ys.append(parse_result.points_hor[i]);
	aJS["hor_ys"] = hor_ys;
}

void get_points(const HttpRequestPtr &request, Callback &&callback)
{
	//std::cout << "get_points request!" << std::endl;
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

void http_init()
{
	std::cout << "html_document_root = " << html_document_root << std::endl;

	drogon::app()
		.setLogLevel(trantor::Logger::LogLevel::kFatal)
		.setThreadNum(1)
		.enableServerHeader(false)
		.setDocumentRoot(html_document_root)
		.addListener("0.0.0.0", 8000);
	//
	drogon::app().registerHandler("/get_params", &get_params, { Get, Post, Options });
	drogon::app().registerHandler("/apply_params", &apply_params, { Get, Post, Options });
	drogon::app().registerHandler("/save_params", &save_params, { Get, Post, Options });
	drogon::app().registerHandler("/get_points", &get_points, { Get, Post, Options });
	//
	drogon::app().run();

}

void http_quit()
{

	//if (drogon::app().isRunning())
		drogon::app().quit();

}
