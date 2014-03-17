SUBDIRS = 3rdlib lib network network/N network/S network/D network/U network/POSS api/ php_client/
all:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	echo "Making all in $$list"; \
	(cd $$subdir && make); \
	done;

clean:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
	echo "Making all in $$list"; \
	(cd $$subdir && make clean); \
	done;

install:
	rm -rf /home/pfs/*;
	mkdir /home/pfs/bin -p;
	mkdir /home/pfs/log -p;
	mkdir /home/pfs/conf -p;
	mkdir /home/pfs/data -p;
	mkdir /home/pfs/path/tmpdir -p;
	mkdir /home/pfs/path/outdir -p;
	mkdir /home/pfs/path/datadir -p;
	mkdir /home/pfs/path/workdir -p;
	mkdir /home/pfs/path/indir -p;
	mkdir /home/pfs/path/bkdir -p;
	mkdir /home/pfs/path/delfile -p;
	cd network; cp pfs_master /home/pfs/bin; cp pfs_master.conf /home/pfs/conf; cp *.txt /home/pfs/bin;
	cd network/N; cp *.so /home/pfs/bin
	cd network/S; cp *.so /home/pfs/bin
	cd network/D; cp *.so /home/pfs/bin
	cd network/U; cp *.so /home/pfs/bin
	cd network/POSS; cp *.so /home/pfs/bin
	cp script/*.sh /home/pfs/bin

