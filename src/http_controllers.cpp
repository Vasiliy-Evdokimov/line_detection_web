#include <iostream>

#include <drogon/drogon.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "defines.hpp"
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
	//
	ConfigData buf = *config_sm_ptr;
	//
	ret["01_CAM_ADDR_1"] = buf.CAM_ADDR_1;
	ret["02_CAM_ADDR_2"] = buf.CAM_ADDR_2;
	//
	ret["03_UDP_ADDR"] = buf.UDP_ADDR;
	ret["04_UDP_PORT"] = buf.UDP_PORT;
	//
	ret["05_NUM_ROI"] = buf.NUM_ROI;
	ret["06_NUM_ROI_H"] = buf.NUM_ROI_H;
	ret["07_NUM_ROI_V"] = buf.NUM_ROI_V;
	//
	ret["08_SHOW_GRAY"] = buf.SHOW_GRAY;
	ret["09_DRAW_DETAILED"] = buf.DRAW_DETAILED;
	ret["10_DRAW_GRID"] = buf.DRAW_GRID;
	ret["11_DRAW"] = buf.DRAW;
	//
	ret["12_MIN_CONT_LEN"] = buf.MIN_CONT_LEN;
	ret["13_HOR_COLLAPSE"] = buf.HOR_COLLAPSE;
	//
	ret["14_GAUSSIAN_BLUR_KERNEL"] = buf.GAUSSIAN_BLUR_KERNEL;
	ret["15_MORPH_OPEN_KERNEL"] = buf.MORPH_OPEN_KERNEL;
	ret["16_MORPH_CLOSE_KERNEL"] = buf.MORPH_CLOSE_KERNEL;
	//
	ret["17_THRESHOLD_THRESH"] = buf.THRESHOLD_THRESH;
	ret["18_THRESHOLD_MAXVAL"] = buf.THRESHOLD_MAXVAL;
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

			str = v["CAM_ADDR_1"].asString();
			strcpy(buf.CAM_ADDR_1, str.c_str());
			str = v["CAM_ADDR_2"].asString();
			strcpy(buf.CAM_ADDR_2, str.c_str());
			//
			str = v["UDP_ADDR"].asString();
			strcpy(buf.UDP_ADDR, str.c_str());
			buf.UDP_PORT = stoi(v["UDP_PORT"].asString());
			//
			buf.NUM_ROI = stoi(v["NUM_ROI"].asString());
			buf.NUM_ROI_H = stoi(v["NUM_ROI_H"].asString());
			buf.NUM_ROI_V = stoi(v["NUM_ROI_V"].asString());
			//buf.recount_data_size();
			//
			buf.SHOW_GRAY = stoi(v["SHOW_GRAY"].asString());
			buf.DRAW_DETAILED = stoi(v["DRAW_DETAILED"].asString());
			buf.DRAW_GRID = stoi(v["DRAW_GRID"].asString());
			buf.DRAW = stoi(v["DRAW"].asString());
			//
			buf.MIN_CONT_LEN = stoi(v["MIN_CONT_LEN"].asString());
			buf.HOR_COLLAPSE = stoi(v["HOR_COLLAPSE"].asString());
			//
			buf.GAUSSIAN_BLUR_KERNEL = stoi(v["GAUSSIAN_BLUR_KERNEL"].asString());
			buf.MORPH_OPEN_KERNEL = stoi(v["MORPH_OPEN_KERNEL"].asString());
			buf.MORPH_CLOSE_KERNEL = stoi(v["MORPH_CLOSE_KERNEL"].asString());
			//
			buf.THRESHOLD_THRESH = stoi(v["THRESHOLD_THRESH"].asString());
			buf.THRESHOLD_MAXVAL = stoi(v["THRESHOLD_MAXVAL"].asString());
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

void get_points(const HttpRequestPtr &request, Callback &&callback)
{
	//std::cout << "get_points request!" << std::endl;
	//
	Json::Value root;
	Json::Value child;
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
