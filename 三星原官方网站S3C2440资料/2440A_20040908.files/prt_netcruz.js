<!--
// Last Modified On : 2004.02.17
// Script Version   : v1.01
//

var s=new Date();
var ServerIp = "210.118.113.223"
var port = "80"

function nTcp(tr) 		// TCP �� �� ȣ���Ѵ�. 
{	
	LoadTime = (new Date()).getTime() - s.getTime();		
		(new Image()).src='http://' + ServerIp + ':' + port + '/cgi-bin/netAgent.cgi?r='
					+ (LoadTime + '&d=' + (new Date()).getTime() + '&x=' + tr );
}

function nStart() 		// �α��� ���� ù ȭ�鿡�� ȣ���Ѵ�. 
{
	 var today = new Date();
	 var the_date = today.setTime(today.getTime() + 60000) ; //60�� ���ĸ� ��Ű ����
	 var the_cookie_date = today.toGMTString(the_date);
     
	document.cookie = "StartTime=" + (new Date().getTime()) + ";path=/; expires="+the_cookie_date;
}

function nStop(tr)  		// ��ȯ�� ���������� ȣ���Ѵ�. 
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
	var strtoday = today.setTime(today.getTime() - 1000); // ���� �ð����� ������ �����Ϸ� �����ش�.

	document.cookie = name + '=; path=/; expires=' + today.toGMTString(strtoday);	
}
//-->
