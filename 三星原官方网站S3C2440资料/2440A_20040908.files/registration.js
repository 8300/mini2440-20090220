/**
 * registration.js
 * 
 * this file included by login/login_registration.asp,
 *                       login/login_modification.asp.
 *
 * @author  Cholhee Lee <cholhee.lee@partner.samsung.com>
 * @version 1.1, 10/02/2003
 */

var LSC_windowName = "SAMSUNG Semiconductor Login";
var LSC_confirmedUserID = null;
var useridChecked = false;

function LSC_openCenterWin(pageUrl, winName, w, h, scroll)
{
	var winl = (screen.width - w) / 2;
	var wint = (screen.height - h) / 2;
	var winProps = 'height='+h+',width='+w+',top='+wint+',left='+winl+',scrollbars='+scroll+',noresiz';
	var win = window.open(pageUrl, winName, winProps)
	if (parseInt(navigator.appVersion) >= 4) {
		win.window.focus();
	}
}

function LSC_openLoginWin(pageUrl)
{
	LSC_openCenterWin(pageUrl, "loginWin", 480, 213, "no");
	window.name = LSC_windowName;
}

function LSC_checkLoginForm(f)
{
	var formObj ;
	if(f == "1")
		formObj = document.loginForm;
	else
		formObj = document.thanksForm;
	
	//alert(document.loginForm.email.value);
	if (!LSC_isValidPassword(formObj.passwd.value))
	{
		formObj.passwd.focus();
		return false;
	}
	return true;
}

function LSC_openRegistrationWin(mode)
{
	var pageUrl = "login_registration.asp?family_cd=" + document.loginForm.family_cd.value + "&partnumber=" + document.loginForm.partnumber.value + "&mode=" + mode;
	opener.location.href = pageUrl;
	self.close();
}

function LSC_openRegistrationWin1()
{
	var pageUrl = "login_registration.asp?family_cd=" + document.thanksForm.family_cd.value + "&partnumber=" + document.thanksForm.partnumber.value + "&mode=" + mode;
	opener.location.href = pageUrl;
	self.close();
}

function LSC_openSearchForgottenPasswordWin()
{
	var f = document.loginForm;
	var pageUrl = "forgot_pass.asp";
	f.action = pageUrl;
	SecuiSubmitEx(f);
}

function LSC_openSearchForgottenPasswordWin1()
{
	var f = document.thanksForm;
	var pageUrl = "forgot_pass.asp";
	f.action = pageUrl;
	SecuiSubmitEx(f);
}

function LSC_doLogin(f)
{
	if (LSC_checkLoginForm(f))
	{
		if( f == "1")
			LSC_doSubmit("login");
		else
			LSC_doSubmit("login1");
	}
	
}

function LSC_doConfirmName()
{
	var f = document.forgotpass
	if(f.firstname.value == "")
	{
		alert("Please input your First Name !!");
		return false;
	}
	if(f.lastname.value == "")
	{
		alert("Please input your Last Name !!");
		return false;
	}
	SecuiSubmitEx(f);
}

function LSC_openModificationWin()
{
	var f = "1";
	if (LSC_checkLoginForm(f))
	{
		LSC_doSubmit("modification");
		self.close();
	}
}

function LSC_openSecessionWin()
{
	var f=  "1";
	if (LSC_checkLoginForm(f))
		if (window.confirm("Really?"))
		{
			LSC_doSubmit("secession");
			self.close();
		}
}

function LSC_doSubmit(actID)
{
	var formObj = document.loginForm;
	
	switch (actID)
	{
		case "login":
			formObj.action = "login_check.asp";
			formObj.target = "_self";
		break;

		case "login1":
			formObj = document.thanksForm;
			formObj.action = "login_check.asp";
			formObj.target = "_self";
		break;
		
		case "modification":
			formObj.action = "login_modification.asp";
			formObj.target = LSC_windowName;
		break;

		case "secession":
			formObj.action = "login_secession.asp";
			formObj.target = LSC_windowName;
		break;
	}

	formObj.method = "POST";
	SecuiSubmitEx(formObj);
}

function LSC_isValidUserID(userid)
{
	if (userid == "")
	{
		alert("Please input ID.");
		return false;
	}
	if (userid.length < 6 || userid.length > 20)
	{
		alert("Length of User ID must be between 6 to 20.");
		return false;
	}
	var re = /[^a-z0-9]/;
	if (re.test(userid))
	{
		alert("Userid must contain only lowercase alpha, numeric characters.");
		return false;
	}
	LSC_confirmedUserID = userid;

	return true;
}




function LSC_isValidPassword(passwd)
{
	if (passwd == "")
	{
		alert("Please input password.");
		return false;
	}
	if (passwd.length < 6 || passwd.length > 20)
	{
		alert("Length of Password must be between 6 to 20.");
		return false;
	}
	if (passwd == LSC_confirmedUserID)
	{
		alert("Userid and password are same.")
		return false;
	}
	if (LSC_dupTest(passwd,4))
	{
		alert("More than 4 same characters exist.");
		return false;
	}
	if (!passwd.match(/[a-zA-Z]/) || !passwd.match(/[0-9]/))
	{
		alert("You must mingle alpha with numeric characters at password.");
		return false;
	}

	return true;
}

function LSC_dupTest(str,maxDup)
{
	var dupCount = 1, lastChar = '', ch;

	for (var i = 0; i < str.length; i++)
	{
		ch = str.charAt(i);
		if (ch == lastChar)
		{
			dupCount++
			if (dupCount >= maxDup)
				return true;
		}
		else
		{
			lastChar = ch;
			dupCount = 1;
		}
	}

	return false;
}

function LSC_doSubmitFP()
{
	var f = document.passwdSearchForm;
	
	if (f.question.value == "")
	{
		alert("Please input security question.");
		f.question.focus();
		return;
	}
	if (f.answer.value == "")
	{
		alert("Please input your answer.");
		f.answer.focus();
		return;
	}
	SecuiSubmitEx(f);
}

function doDisagree()
{
	if (window.confirm("Really?"))
		GoUrl("home");
}

function doAgree(f)
{
	var formObj;
	if(f=="1")
		formObj = document.registForm;
	else
		formObj = document.registForm1;
//	var f = document.registForm;
	if (formObj.mode.value == "modify")
	{
		useridChecked = true;
//		alert("useridChecked = true");
	}
	
	if (checkRRegistForm(formObj, f))
	{
		SecuiSubmitEx(formObj);
	}
}


function openIDSearchWin()
{
	var objUserid = document.registForm.email;
	if (LSC_isValidEmail(objUserid.value))
	{
		var pageUrl = "login_idsearch.asp"
					+ "?email=" + objUserid.value;
		LSC_openCenterWin(pageUrl, "idSearchWin", 480, 315, "no");
	}
	else
	{
		alert("You can't use this email address!");
		objUserid.focus();
	}
}

function LSC_isValidEmail(email)
{
	var strs = email.split('@');
	if (strs.length == 2)
	{
		return true;
	}
	else
	{
		return false;
	}

}
function checkRRegistForm(f, obj)      
{
	if(obj == "1")
	{
	if (f.firstName.value == "")
	{
		alert("Please input first name.");
		f.firstName.focus();
		return false;
	}
	if (f.lastName.value == "")
	{
		alert("Please input last name.");
		f.lastName.focus();
		return false;
	}
	
	var aa = LSC_isValidEmail(f.email.value);
	if (!aa)
	{
		alert("email is not valid");
		f.email.focus();
		return false;
	}
	if(f.pagemode.value == "page1"){
	if (useridChecked == false)
	{
		alert("Please do 'ID Search'");
		return false;
	}
	if (f.email.value != f.email2.value)
	{
		alert("Mismatch Your Email!");
		f.email2.value = "";
		f.email2.focus();
		return false;
	}
	if (f.passwd.value != f.passwd2.value)
	{
		alert("Mismatch password!");
		f.passwd2.focus();
		return false;
	}
	if (!LSC_isValidPassword(f.passwd.value))
	{
		f.passwd.focus();
		return false;
	}
}
	
	if (f.company.value == "")
	{
		alert("Please input your company");
		f.company.focus();
		return false;
	}
	if (f.JobTitle.value == "")
	{
		alert("Please input Job Title");
		f.JobTitle.focus();
		return false;
	}
	if (f.JobFunction.value == "")
	{
		alert("Please Select Job Function");
		f.JobFunction.focus();
		return false;
	}
}
	return true;
}


