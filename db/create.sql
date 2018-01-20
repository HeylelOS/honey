/*
	create.sql
	Copyright (c) 2018, Valentin Debon

	This file is part of the Honey package manager
	subject the BSD 3-Clause License, see LICENSE.txt
*/

create table Geister(
	name text primary key
);

create table Packages(
	version text,
	name text,

	foreign key(name) references Geister(name),

	primary key(version, name)
);

create table Replaces (
	name text,
	gname text,

	foreign key(name) references Geister(name),
	foreign key(gname) references Geister(name),

	primary key(name, gname)
);

create table Dependencies (
	name text,
	dname text,

	foreign key(name) references Geister(name),
	foreign key(dname) references Geister(name),

	primary key(name, dname)
);

