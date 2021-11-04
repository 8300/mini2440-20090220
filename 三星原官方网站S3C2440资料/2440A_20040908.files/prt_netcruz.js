<!--
// Last Modified On : 2004.02.17
// Script Version   : v1.01
//

var s=new Date();
var ServerIp = "210.118.113.223"
var port = "80"

function nTcp(tr) 		// TCP 일 때 호출한다. 
{	
	LoadTime = (new Date()).getTime() - s.getTime();		
		(new Image()).src='http://' + ServerIp + ':' + port + '/cgi-bin/netAgent.cgi?r='
					+ (LoadTime + '&d=' + (new Date()).getTime() + '&x=' + tr );
}

function nStart() 		// 로그인 같은 첫 화면에서 호출한다. 
{
	 var today = new Date();
	 var the_date = today.setTime(today.getTime() + 60000) ; //60초 이후면 쿠키 만료
	 var the_cookie_date = today.toGMTString(the_date);
     
	document.cookie = "StartTime=" + (new Date().getTime()) + ";path=/; expires="+the_cookie_date;
}

function nStop(tr)  		// 전환된 페이지에서 호출한다. 
{
	var StartTime = GetCookie("StartTime");			
	if(StartTime)
	{
	   var today = new Date();			
	   var LoadTime = today.getTime() - StartTime;		
	
	   (new Image()).src='http://' + ServerIp + ':' + port + '/cgi-bin/netAgent.cgi?r='
			+ (LoadTime + '&d=' + (new Date()).getTime() + '&x=' + tr );
	}
	SetCookie("StartTime");
}

function GetCookie(name)  	// Internal Function
{
	var index = document.cookie.indexOf(name + "=");	
	if(index == -1) return null;	
	index = document.cookie.indexOf("=",index) + 1;
	
	var endstr = document.cookie.indexOf(";",index);
	if(endstr == -1)
	   endstr = document.cookie.length;
	   return unescape(document.cookie.substring(index,endstr));		
}

function SetCookie(name)	// Internal Function
{
	var today = new Date();
	var strtoday = today.setTime(today.getTime() - 1000); // 현재 시간보다 이전을 만료일로 정해준다.

	document.cookie = name + '=; path=/; expires=' + today.toGMTString(strtoday);	
}
//-->
