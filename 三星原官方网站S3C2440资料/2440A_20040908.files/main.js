//var page = ["http://www.samsung.com/index.htm","http://www.samsung.com/AboutSAMSUNG/ValuesPhilosophy/index.htm","http://www.samsung.com/AboutSAMSUNG/ValuesPhilosophy/DigitalVision/index.htm","http://www.samsung.com/AboutSAMSUNG/CEO/Chairman/CEOMessage/index.htm","http://www.samsung.com/AboutSAMSUNG/CEO/Chairman/PersonalProfile/index.htm","http://www.samsung.com/AboutSAMSUNG/CEO/Chairman/InthePress/index.htm","http://www.samsung.com/AboutSAMSUNG/CEO/ViceChairman/PersonalProfile/index.htm","http://www.samsung.com/Sitemap/Sitemap.htm","http://www.samsung.com/ContactUs/ContactUs.htm","http://www.samsung.com/AboutSAMSUNG/InvestorRelations/","http://www.samsung.com/AboutSAMSUNG/Careers/","http://www.samsung.com/BusinessPartners/","http://www.samsung.com/Products/Semiconductor/"]	//제외 될 페이지
var page = ["/AboutSAMSUNG/ValuesPhilosophy/index.htm","/AboutSAMSUNG/ValuesPhilosophy/DigitalVision/index.htm","/AboutSAMSUNG/CEO/Chairman/CEOMessage/index.htm","/AboutSAMSUNG/CEO/Chairman/PersonalProfile/index.htm","/AboutSAMSUNG/CEO/Chairman/InthePress/index.htm","/AboutSAMSUNG/CEO/ViceChairman/PersonalProfile/index.htm","/Sitemap/Sitemap.htm","/ContactUs/ContactUs.htm","/AboutSAMSUNG/InvestorRelations/","/AboutSAMSUNG/Careers/","/BusinessPartners/","/Products/Semiconductor/","/Products/TFTLCD/"]	//제외 될 페이지

var url = location.pathname;
var sw = true;
if (url == "/"||url == "") {
	url = "/index.htm";
}

for (i=0;i<page.length;i++) {
	if (url.toLowerCase()=="/index.htm"||url.toLowerCase()=="/index_01.htm"||url.toLowerCase()=="/index_02.htm"||url.toLowerCase()=="/index_03.htm")
	{
		sw = false;
		break;
	}
	if (url.toLowerCase().indexOf(page[i].toLowerCase()) != -1) {
		sw = false;
		break;
	}
}
if (sw) {
	document.write ("<table border='0' cellpadding='0' cellspacing='0'>");
	document.write ("	<tr><td height='50'></td></tr>");
	document.write ("	<tr><td style='padding-left:160'>");
	document.write ("		<table width='609' border='0' cellpadding='0' cellspacing='0' style='padding:10'>");
	document.write ("			<tr><td height='2' bgcolor='#A3A3A3'></td></tr>");
	document.write ("			<tr><td bgcolor='#F8F8F8'>");
	document.write ("				<table border='0' cellpadding='0' cellspacing='0'>");
	document.write ("					<tr><td width='100%'>");
	document.write ("						<table border='0' cellpadding='0' cellspacing='0'>");
	document.write ("						<form name='varform' method='post' action='/satisfaction/question2_01.asp'>");
	document.write ("							<tr><td><img src='/satisfaction/images/arrow_o.gif' width='3' height='5' align='absmiddle'> <b>The information on this page meets my needs.</b></td></tr>");
	document.write ("							<tr><td height='5'></td></tr>");
	document.write ("							<tr><td style='padding-left:7'>");
	document.write ("									<b style='color:#007CD4'>Agree</b>");
	document.write ("									<input type='radio' name='Q1' value='1' style='margin:0 0 -3 10;background-color:#F8F8F8'>");
	document.write ("									<input type='radio' name='Q1' value='2' style='margin:0 0 -3 10;background-color:#F8F8F8'>");
	document.write ("									<input type='radio' name='Q1' value='3' style='margin:0 0 -3 10;background-color:#F8F8F8'>");
	document.write ("									<input type='radio' name='Q1' value='4' style='margin:0 0 -3 10;background-color:#F8F8F8'>");
	document.write ("									<input type='radio' name='Q1' value='5' style='margin:0 10 -3 10;background-color:#F8F8F8'>");
	document.write ("									<b style='color:#007CD4'>Disagree</b>");
	document.write ("								</td></tr>");
	document.write ("							<input type='hidden' name='url'>");
	document.write ("						</form>");
	document.write ("						</table>");
	document.write ("						</td>");
	document.write ("						<td valign='bottom'><a href='javascript:check()'><img src='/satisfaction/images/btn_submit.gif' width='57' height='14' border='0'></a></td>");
	document.write ("					</tr>");
	document.write ("				</table>");
	document.write ("				</td></tr>");
	document.write ("			<tr><td height='2' bgcolor='#E8E8E8'></td></tr>");
	document.write ("		</table>");
	document.write ("		</td></tr>");
	document.write ("</table>");

	url = location.pathname;
	document.varform.url.value=url;
}

function check() {
	var obj = document.varform;
	sw = check_radio(obj);
	if (sw) {
		win = window.open("about:blank","win_01","width=377,height=200");
		obj.target = "win_01";
		obj.submit();
	}
}
