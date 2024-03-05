const sever_url = location.origin;

var app = new Vue({
    el: '#app',
    data: {
        params_init: false,
        cur_params: {},
        new_params: {},
        status_ok: true,
        status_text: "",
        //
        draw_canvas: null,
        draw_context: null,
        draw_interval: null,
        //
        debug_canvas: null,
        debug_context: null,
        //
        web_show_lines: false,
        web_show_debug: false,
        web_show_image: false,
        web_interval: 100,
        //
        use_image_roi: false,
        image_roi_x: 0,
        image_roi_y: 0,
        image_roi_w: 0,
        image_roi_h: 0,        
        //
        params_descriptions: new Map()
    },
    methods: {
        get_param_by_name: function(aParams, aName) {
            for (key in aParams)
                if (key.slice(3) == aName)
                    return aParams[key];                
            return null;
        },
        process_params_data: function(data) {
            let res = {};
            let new_key = '';
            for (key in data) {
                new_key = key.slice(3);
                res[new_key] = data[key];
            }
            return res;
        },
        server_request(aStatusText, aURL, aMethod, aDataObj, aCallback, aApplyStatus = true, aLogResponse = true) {
            if (aApplyStatus)
                this.status_text = aStatusText;
            //
            var request = new Request(sever_url + aURL);
            var init = {
                method: aMethod,
                headers: { "Content-Type": "application/json" }
            };
            if (aMethod == "POST")
                init.body = JSON.stringify(aDataObj);

            fetch(request, init)
            .then((response) => response.json())
            .then((data) => {
                if (aLogResponse)
                    console.log(JSON.stringify(data));
                if (data.message == "error") throw "error";
                aCallback(data);
            })
            .then(() => {
                if (aApplyStatus) {
                    this.status_ok = true;                    
                }
            })
            .catch((err) => {
                if (aApplyStatus) {
                    this.status_ok = false;                    
                }                    
            });
        },        
        get_params: function(aApplyStatus = true) {
            this.server_request(
                "Получение текущих параметров...",
                "/get_params",
                "GET",
                null,
                this.get_params_callback,
                aApplyStatus
            );           
        },
        get_params_callback: function(data) {
            this.cur_params = JSON.parse(JSON.stringify(data));
            if (!this.params_init) {
                this.params_init = true;
            }
            this.new_params = JSON.parse(JSON.stringify(this.cur_params));            
            this.check_params();
        },
        get_config_map: function() {
            this.server_request(
                "Получение описаний параметров...",
                "/get_config_map",
                "GET",
                null,
                this.get_config_map_callback
            );           
        },
        get_config_map_callback: function(data) {
            let params = JSON.parse(JSON.stringify(data));
            this.params_descriptions.clear();
            for (item of params.result)
                this.params_descriptions.set(item.name,
                    { "type": item.type, "descr": item.descr });
        },
        check_params: function()
        {
            this.use_image_roi = (this.get_param_by_name(this.cur_params, "USE_IMAGE_ROI") > 0);
            this.image_roi_x = (this.get_param_by_name(this.cur_params, "IMAGE_ROI_X"));
            this.image_roi_y = (this.get_param_by_name(this.cur_params, "IMAGE_ROI_Y"));
            this.image_roi_w = (this.get_param_by_name(this.cur_params, "IMAGE_ROI_W"));
            this.image_roi_h = (this.get_param_by_name(this.cur_params, "IMAGE_ROI_H"));                                    
            //
            this.web_show_lines = (this.get_param_by_name(this.cur_params, "WEB_SHOW_LINES") > 0);
            this.web_show_debug = (this.get_param_by_name(this.cur_params, "WEB_SHOW_DEBUG") > 0);
            this.web_show_image = (this.get_param_by_name(this.cur_params, "WEB_SHOW_IMAGE") > 0);
            this.web_interval = parseInt(this.get_param_by_name(this.cur_params, "WEB_INTERVAL"));
            //
            if (this.draw_interval) 
                clearInterval(this.draw_interval);
            this.draw_interval = setInterval(this.draw, this.web_interval);
        },
        apply_params: function() {    
            let cnv = this.canvas;
            let ctx = this.context;
            //
            if (!(this.check_form(this.new_params)))
                return;
            //
            this.server_request(
                "Применение новых параметров...",
                "/apply_params",
                "POST",
                this.process_params_data(this.new_params),
                this.apply_params_callback
            );
        },
        apply_params_callback: function(data) {
            this.cur_params = JSON.parse(JSON.stringify(this.new_params));
            this.check_params();
        },
        save_params: function() {            
            this.server_request(
                "Сохранение новых параметров...",
                "/save_params",
                "GET",
                null,
                this.save_params_callback
            );
        },
        save_params_callback: function(data) {            
            this.get_params(false);            
        },
        get_points: function() {            
            this.server_request(
                "Получение результатов распознавания...",
                "/get_points",
                "GET",
                null,
                this.get_points_callback,
                false,
                false
            );
        },
        topleft(imgW, imgH, pt)
        {
            let cnt = { x: imgW / 2, y: imgH / 2 };
            return { x: pt.x + cnt.x, y: cnt.y - pt.y };
        },
        get_points_callback: function(data)
        {
            let ctx = this.draw_context;
            //
            var w0 = 0, h0 = 0;
            var w1 = 0, h1 = 0;
            //
            if (data.result) 
            {
                if (data.result[0]) 
                {
                    w0 = data.result[0].width;
                    h0 = data.result[0].height;
                }
                //
                if (data.result[1]) 
                {
                    w1 = data.result[1].width;
                    h1 = data.result[1].height;
                }
            }
            //
            let canvas_width = w0 + w1;
            let canvas_height = Math.max(h0, h1);
            if ((this.draw_canvas.width != canvas_width) ||
                (this.draw_canvas.height != canvas_height)) 
            {
                this.draw_canvas.width = canvas_width;
                this.draw_canvas.height = canvas_height;           
            }
            //
            if (this.web_show_image)
                if ((this.debug_canvas.width != canvas_width) ||
                    (this.debug_canvas.height != canvas_height)) 
                {
                    this.debug_canvas.width = canvas_width;
                    this.debug_canvas.height = canvas_height;           
                }
            //
            let offset = 0;
            for (let i = 0; i < data.result.length; i++) {                

                let res = data.result[i];
                let width = res.width;
                let height = res.height;
                //            
                ctx.beginPath();
                ctx.rect(offset, 0, width, height);
                ctx.closePath();
                ctx.strokeStyle = "yellow";
                ctx.fillStyle = "black";
                ctx.fill();
                ctx.stroke();
                ctx.strokeStyle = "black";
                //
                this.draw_point_text(10 + offset, 20, "lime", "Camera " + res.camera_no);  
                //
                if (res.error_flags > 0) {
                    if ((res.error_flags & 1) > 0) {    //  fl_err_line
                        this.draw_flag_arc(50 + offset, "red");
                    }
                    if ((res.error_flags & 4) > 0) {    //  fl_err_camera
                        this.draw_flag_text(170 + offset, 50, "red", "Camera error!");
                    }
                    if ((res.error_flags & 8) > 0) {    //  result timeout
                        this.draw_flag_text(170 + offset, 80, "magenta", "Camera timeout!");
                    }
                } else {
                    ctx.lineWidth = 3;                    
                    if (res.res_points) 
                    {
                        prev_pt = { x: (width / 2) + offset, y: height };
                        for (let j = 0; j < res.res_points.length; j++) 
                        {
                            res_point = res.res_points[j];                        
                            //
                            ctx.strokeStyle = "green";
                            ctx.fillStyle = "green";
                            ctx.beginPath();
                            ctx.moveTo(prev_pt.x, prev_pt.y);                                 
                            draw_pt = this.topleft(width, height, res_point);
                            draw_pt.x += offset;
                            ctx.lineTo(draw_pt.x, draw_pt.y);
                            ctx.closePath();
                            ctx.stroke();
                            ctx.fill();
                            //
                            this.draw_point_text(draw_pt.x + 10, draw_pt.y, "yellow", "(" + res_point.x + ";" + res_point.y + ") px");                            
                            //
                            prev_pt.x = draw_pt.x;
                            prev_pt.y = draw_pt.y;
                            //
                            this.draw_point_arc(draw_pt, "yellow");
                        }                         
                    }     
                    //
                    if (res.center_x != undefined)
                    {
                        draw_pt = this.topleft(width, height, { x: res.center_x, y: 0 });
                        draw_pt.x += offset;
                        this.draw_point_arc(draw_pt, "magenta");
                        this.draw_point_text(draw_pt.x + 10, draw_pt.y, "magenta", res.center_x + " px");
                        this.draw_point_text(draw_pt.x + 10, draw_pt.y + 15, "magenta", res.center_x_mm + " mm");
                    }
                    //
                    if (res.hor_ys) 
                    {                        
                        for (let j = 0; j < res.hor_ys.length; j++) 
                        {
                            hor_y = res.hor_ys[j];                            
                            //                            
                            ctx.strokeStyle = "red";
                            draw_pt = this.topleft(width, height, { x: 0, y: hor_y });				            
                            ctx.beginPath();
                            ctx.moveTo(offset, draw_pt.y);
                            ctx.lineTo(width + offset, draw_pt.y);
                            ctx.closePath();
                            ctx.stroke();
                            //
                            this.draw_point_text(10 + offset, draw_pt.y + 15, "red", hor_y + " px");                            
                        }
                    }                        
                }
                //
                if ((res.zone_flags & 4) > 0)   // fl_slow_zone
                    this.draw_flag_arc(80 + offset, "green");
                //
                if ((res.zone_flags & 2) > 0)   // fl_stop_zone
                    this.draw_flag_arc(110 + offset, "blue");
                //
                if ((res.zone_flags & 1) > 0) { // fl_stop_mark
                    let clr = "cyan";
                    this.draw_flag_arc(140 + offset, clr);                    
                    ctx.fillStyle = clr;
                    ctx.strokeStyle = ctx.fillStyle;
                    ctx.font = "italic 30pt Arial";
                    ctx.fillText(res.stop_distance, 170 + offset, 50);
                }    
                //
                if (this.web_show_debug)
                {
                    ctx.fillStyle = "magenta";
                    ctx.strokeStyle = ctx.fillStyle;
                    ctx.font = "15pt Arial";
                    ctx.fillText("hidro_height = " + res.hidro_height, 325 + offset, 20); 
                    //
                    let threshold_thresh = Number(this.get_param_by_name(this.cur_params, "THRESHOLD_THRESH"));
                    let threshold_height_k = Number(this.get_param_by_name(this.cur_params, "THRESHOLD_HEIGHT_K"));
                    threshold_height_k /= 100;                
                    ctx.fillText("threshold  = " + Math.round(threshold_thresh + res.hidro_height * threshold_height_k),                    
                        325 + offset, 40);
                }        
                //
                if ((res.pult_flags & 1) > 0) 
                {
                    ctx.fillStyle = "yellow";
                    ctx.strokeStyle = ctx.fillStyle;
                    ctx.font = "bold 15pt Arial";
                    if (this.get_param_by_name(this.cur_params, "AUTO_EMULATE") == 1)
                        ctx.fillText("AUTO(E)", 550 + offset, 20);
                    else
                        ctx.fillText("AUTO", 575 + offset, 20);
                }
                //
                if (this.web_show_debug && data.debug)
                {
                    let dbg = data.debug[i];
                    //
                    let roi = this.get_param_by_name(this.cur_params, "NUM_ROI");
                    let roi_offset = height / roi;
                    ctx.strokeStyle = "yellow";
                    ctx.lineWidth = 1;
                    //                    
                    for (let j = 1; j < roi; j++)
                    {
                        ctx.beginPath();
                        let y = roi_offset * j;
                        ctx.moveTo(offset, y);                                 
                        ctx.lineTo(offset + width, y); 
                        ctx.closePath();
                        ctx.stroke();                       
                    }                    
                    //
                    //  center lines
                    ctx.strokeStyle = "magenta";
                    ctx.lineWidth = 1;
                    //
                    ctx.beginPath();
                    let x = width / 2 + offset;
                    ctx.moveTo(x, 0);                                 
                    ctx.lineTo(x, height);
                    //
                    let y = height / 2;
                    ctx.moveTo(0 + offset, y);                                 
                    ctx.lineTo(width + offset, y);
                    ctx.closePath();
                    ctx.stroke();
                    //
                    if (this.use_image_roi)
                    {
                        //
                        ctx.lineWidth = 1;
                        ctx.strokeStyle = "cyan";
                        ctx.beginPath();
                        ctx.rect(
                            this.image_roi_x + offset,
                            this.image_roi_y,
                            this.image_roi_w,
                            this.image_roi_h
                        );
                        ctx.closePath();
                        ctx.stroke();
                    }
                    //
                    if (dbg.contours)
                    for (let j = 0; j < dbg.contours.length; j++) {
                        var cntr = dbg.contours[j];
                        //
                        var clr = "blue";
                        switch (cntr.type) {
                            case 2:
                                clr = "yellow";
                                break;
                            case 3:
                                clr = "red";
                                break;
                        }
                        //
                        ctx.lineWidth = 1;
                        ctx.strokeStyle = clr;
                        ctx.beginPath();
                        ctx.rect(cntr.left_top.x + offset, cntr.left_top.y, cntr.width, cntr.height);
                        ctx.closePath();
                        ctx.stroke();
                        //
                        this.draw_point_arc({ "x": cntr.center.x + offset, "y": cntr.center.y }, clr);
                        //
                        this.draw_point_text(cntr.left_top.x + offset + 2, cntr.left_top.y + 12, "lime", "L=" + cntr.length); 
                        this.draw_point_text(cntr.left_top.x + offset + 2, cntr.left_top.y + 25, "lime", "W=" + cntr.width);                        
                    }
                }
                //
                if (this.web_show_image && data.debug)
                {
                    let img = data.debug[i].image;
                    //
                    let dctx = this.debug_context;
                    let j = 0;
                    let hex = "";
                    let row = 0, col = 0;
                    dctx.lineWidth = 1;
                    if (img)
                    while (j < img.length)
                    {
                        let dec = parseInt(img.substring(j, j + 2), 16);             
                        //
                        dctx.strokeStyle = (dec & 1) ? "white" : "black";
                        //
                        dctx.beginPath();
                        //
                        dctx.moveTo(col + offset, row);
                        col += (dec >> 1);                                
                        dctx.lineTo(col + offset, row);                        
                        dctx.closePath();
                        dctx.stroke();
                        //
                        if (col >= width - 1) {
                            col = 0;
                            row++;
                        }
                        //
                        j += 2;
                    }
                    //
                    if (this.web_show_debug)
                    {
                        //  center lines
                        dctx.strokeStyle = "magenta";
                        dctx.lineWidth = 1;
                        //
                        dctx.beginPath();
                        let x = width / 2 + offset;
                        dctx.moveTo(x, 0);                                 
                        dctx.lineTo(x, height);
                        //
                        let y = height / 2;
                        dctx.moveTo(0 + offset, y);                                 
                        dctx.lineTo(width + offset, y);
                        dctx.closePath();
                        dctx.stroke();   
                    }                 
                }
                //
                offset = width;

            }            
        },      
        draw_point_arc: function(aPoint, aColor) {            
            let ctx = this.draw_context;
            ctx.strokeStyle = aColor;
            ctx.fillStyle = aColor;
            ctx.beginPath();                                            
            ctx.arc(aPoint.x, aPoint.y, 2, 0, this.get_radians(360));
            ctx.closePath();
            ctx.stroke();
            ctx.fill();
        },
        draw_point_text: function(aX, aY, aColor, aText) {
            let ctx = this.draw_context;                   
            ctx.fillStyle = aColor;
            ctx.strokeStyle = ctx.fillStyle;
            ctx.font = "10pt Arial";
            ctx.fillText(aText, aX, aY);
        },
        draw_flag_arc: function(aX, aColor) {
            let ctx = this.draw_context;
            ctx.beginPath();                                            
            ctx.arc(aX, 50, 20, 0, this.get_radians(360));
            ctx.closePath();
            ctx.strokeStyle = aColor;
            ctx.fillStyle = aColor;
            ctx.stroke();
            ctx.fill();
        },
        draw_flag_text: function(aX, aY, aColor, aText) {
            let ctx = this.draw_context;                   
            ctx.fillStyle = aColor;
            ctx.strokeStyle = ctx.fillStyle;
            ctx.font = "italic 20pt Arial";
            ctx.fillText(aText, aX, aY);
        },
        get_param_info: function(param_name) {
            param_name = param_name.slice(3);
            var res = this.params_descriptions.get(param_name);
            return res;
        },
        is_checkbox_param: function(name) {
            return checkboxes_params.has(name);
        },
        is_numeric: function(str) {
            return !isNaN(str) &&
                   !isNaN(parseFloat(str));
        },
        is_ip_address: function (str) {
            return (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(str));
        },
        check_form: function(params_obj) {
            let err_params = "";
            //
            for (key in params_obj) {
                let value = params_obj[key];
                if (!value) continue;
                let info = this.get_param_info(key);
                if (!info) continue;
                //
                let err_fl = false;
                switch(info.type) {
                    case(1):    // int
                        {
                            err_fl = !(this.is_numeric(value));                                 
                            break;
                        }
                    case(2):    // string
                        {                        
                            break;
                        }
                    case(3):    // bool
                        {
                            err_fl = (!(this.is_numeric(value)) || (value > 2 ));
                            break;
                        }
                    case(4):    //  ip
                        {
                            err_fl = !(this.is_ip_address(value));   
                            break;
                        }                    
                }
                if (err_fl)
                    err_params += info.descr + "; ";
            }
            //
            if (err_params != "") {
                this.status_text = "Некорректые значения следующих параметров: " + err_params;
                this.status_ok = false;                
                return false;
            }
            //
            return true;
        },
        get_radians: function(degrees) {
            return (Math.PI / 180) * degrees;
        },
        draw: function() {
            if (this.web_show_lines)
                this.get_points();
        }
    },
    mounted: function () {
        this.get_config_map();
        this.get_params();
        //
        this.draw_canvas = document.getElementById("result_canvas");
        this.draw_context =  this.draw_canvas.getContext("2d");
        //
        this.debug_canvas = document.getElementById("debug_canvas");
        this.debug_context =  this.debug_canvas.getContext("2d");
    }
});