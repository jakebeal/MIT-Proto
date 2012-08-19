# NOTE: order is important in this list!

if USE_NEOCOMPILER
neopaleodir = neo
else
neopaleodir = paleo
endif

SUBDIRS = \
	core \
	$(neopaleodir) \
	.

protolib_DATA = \
	bootstrap.proto \
	broadcast.proto \
	brownian.proto \
	core.ops \
	core.proto \
	dilate.proto \
	disperse.proto \
	distance-to.proto \
	distance.proto \
	dither.proto \
	elect.proto \
	flip.proto \
	gradcast.proto \
	norm.proto \
	timer.proto

EXTRA_DIST = $(protolib_DATA)
