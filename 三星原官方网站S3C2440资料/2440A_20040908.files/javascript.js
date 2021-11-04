function body_size() {
	var obj = navigator.appVersion;
	var hei = 29;
	if (navigator.appVersion.indexOf("NT") != -1) {
		os = obj.substr(obj.indexOf("NT"),6);
		if (os > "NT 5.0") {
			hei = 35;
		}
	}
	wid = document.body.scrollWidth+10;
	hei = document.body.scrollHeight+hei;
	self.resizeTo(wid,hei);
}


function check_radio(obj) {
	var q_no = 0;
	var sw = false;
	var name;
	for (i=0;i<obj.elements.length;i++) {
		if ((obj.elements[i].type == "radio" || obj.elements[i].type == "checkbox") && name != obj.elements[i].name) {
			sw = false;
			name = obj.elements[i].name;
			q_no++;
			for (j=0;j<obj.elements[name].length;j++) {
				obj_name = eval("obj." + name);
				if (obj_name[j].checked){
					sw = true;
					/*
					if (name == 'Q2_2' && j == 3 && !obj.Q2_2ETC.value) {
						alert("If you checked \"Other\" for NO.2, please input an answer in the textbox.");
						return;
					}
					if (name == 'Q2_2' && j != 3 && obj.Q2_2ETC.value) {
						alert("If you didn't check \"Other\" for NO.2, you must not input an answer in the textbox.");
						return;
					}
					*/
				}
			}
			if (!sw) {
				alert("You need to select an answer before clicking submit.\n Please chose an answer & then click submit");
				break;
			}
		} else {
			name = obj.elements[i].name;
			obj_name = eval("obj." + name);
			if (name == "country" && obj_name.value == "") {
				alert ("You need to select a country.");
				return false;
			}
		}
		
	}
	return sw;
}
