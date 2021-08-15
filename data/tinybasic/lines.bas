5 REM lines
10 CLS
20 FOR a=5 TO 235 STEP 5
21 b = 235 - a
22 LINE  a, 0, 239,  a
23 LINE 0,  a,  a, 239
24 LINE  a, 0, 0, b
25 LINE  a, 239, 239, b
27 NE aT  a 
75 DELAY 2000
80 TExT
90 PRINT "Done"

