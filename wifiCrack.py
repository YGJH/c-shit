import itertools as it
words = "0123456789abcdefghijklmnopqrstuvwxyz"
r = it.product(words, repeat=8)
dic = open("dic.txt", "w")
for i in r:
    dic.write("".join(i) + "\n")
dic.close()
