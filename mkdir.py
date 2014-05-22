import os
import sys

if __name__=='__main__':
	if len( sys.argv ) <= 1:
		sys.exit( "usage: mkdir.py <directory>" )

	for directory in sys.argv[1:]:
		try:
			os.makedirs( directory )
		except OSError:
			pass

