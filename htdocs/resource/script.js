var s=null;function Previous(a){s||(s=document.getElementById("date"));s.selectedIndex<s.length&&(s.selectedIndex+=1,Go(a,s))}function Next(a){s||(s=document.getElementById("date"));0<s.selectedIndex&&(--s.selectedIndex,Go(a,s))}function Go(a,b){s||(s=b);document.location="view.php?camera="+encodeURIComponent(a)+"&recording="+encodeURIComponent(s.value)}document.onreadystatechange=function(){var a=null,b=null;"complete"==document.readyState&&(null==s&&(s=document.getElementById("date")),a=document.getElementById("previous"),b=document.getElementById("next"),null!=a&&null!=b&&null!=s&&(a.disabled=s.selectedIndex==s.length,b.disabled=0==s.selectedIndex))};function Go2(a,b){document.location="view.php?camera="+encodeURIComponent(a)+"&recording="+encodeURIComponent(b)};