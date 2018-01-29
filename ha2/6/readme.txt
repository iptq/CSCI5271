For this challenge, I exposed the key character by character starting from the
end, using this method:

1. Start with a 19-character string.
2. Send this string to the server and get its hash.
3. Loop through chars 0-255, for the brute-force character. Compute (origstring + keysegment + newcharacter).
4. Take this computation and send it to the server and retrieve the hash.
5. There will be exactly 1 matching character. Add this to the key.
6. Shorten the string by 1 character, and the go back to step 2 until the string is empty.

By this method, each character is exposed 1 by 1. The key that I retrieved was
'GbbElypm0ztVpceOjVre'.
