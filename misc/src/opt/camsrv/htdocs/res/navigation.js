var s=null;function Previous(){s||(s=document.getElementById("date"));s.selectedIndex<s.length&&(s.selectedIndex+=1,Go(s))}function Next(){s||(s=document.getElementById("date"));0<s.selectedIndex&&(--s.selectedIndex,Go(s))}function Go(a){s||(s=a);document.location="/?camera="+encodeURIComponent(camera)+"&recording="+encodeURIComponent(s.value)}function Download(a){var b;b=-1!=a.indexOf("/")?camera+"-"+a.substring(a.lastIndexOf("/")+1):camera+"-"+a;var c=document.createElement("a");c.download=b;c.href=a;c.click()}document.onreadystatechange=function(){var a=null,b=null;"complete"==document.readyState&&(null==s&&(s=document.getElementById("date")),null==a&&(a=document.getElementById("previous")),null==b&&(b=document.getElementById("next")),null!=a&&null!=b&&null!=s&&(a.disabled=s.selectedIndex==s.length,b.disabled=0==s.selectedIndex))};