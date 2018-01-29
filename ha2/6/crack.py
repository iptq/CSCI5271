import requests
import string
import sys


def gethash(uname):
    r = requests.get("https://192.168.16.1/mac-cookie.php?username={}".format(uname), verify=False)
    return r.text.strip().split("with the MAC ")[1].split(".")[0]


username = "olololololololololololololololo"[:19]
key = ""

for i in range(19, -1, -1):
    userseg = username[:i]
    keyseg = key[:(19 - i)]
    basehash = gethash(userseg)
    for c in string.printable:
        cand = userseg + keyseg + c
        if basehash == gethash(cand):
            key += c
            print "added '{}'".format(c)
            break
    else:
        print "failed on i={}".format(i)
        sys.exit(1)
print "key = '{}'".format(key)
