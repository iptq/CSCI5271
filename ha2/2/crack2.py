import md5

username="seoh16"
realm="Cheese"
nonce="2kiHAWxdBQA=117175983f2816893c1e24a382fbe93188ae5b4f"
uri="/secret/cheese.php"
cnonce="MDIzY2VhMzk1MjVkNDU5MGVjMTEyYWRmNzJhMzkwZDc="
nc="00000001" 
qop="auth"
response="40ce378475bc8a64d33a19902b757b85"

ha2 = md5.new("HEAD:"+uri)
f = open("usr/share/dict/words").read().split()
for word in f:
	ha1 = md5.new()
	ha1.update(username+":"+realm+":"+word)
	result = md5.new()
	result.update(ha1.hexdigest()+":"+nonce+":"+nc+":"+cnonce+":"+qop+":"+ha2.hexdigest())
	if result.hexdigest() == response:
		print word
		break
