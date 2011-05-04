import dmultitouch
import random

dmultitouch.setHeightOfTouch(7)
dmultitouch.setImageSize(200,200)
N = 200 * 200
s = ''.join('z' for x in range(N))
dmultitouch.setBase(s)
for i in range(20):
	s = ''.join(random.choice('ZKA90adhijnopqrstu') for x in range(N))
	dmultitouch.update(s)
	print(dmultitouch.getTouches())
#dmultitouch.getConnectedRegions()

