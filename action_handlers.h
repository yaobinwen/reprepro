#ifndef REPREPRO_ACTION_HANDLERS_H
#define REPREPRO_ACTION_HANDLERS_H

#include "atoms.h"
#include "macros_action.h"
#include "macros_attributes.h"
#include "retvalue.h"

ACTION_N(n, n, y, printargs);
ACTION_N(n, n, n, dumpuncompressors);
ACTION_N(n, n, y, uncompress);
ACTION_N(n, n, y, extractcontrol);
ACTION_N(n, n, y, extractfilelist);
ACTION_N(n, n, y, extractsourcesection);
ACTION_F(n, n, n, y, fakeemptyfilelist);
ACTION_F(n, n, n, y, generatefilelists);
ACTION_T(n, n, translatefilelists);
ACTION_N(n, n, n, translatelegacychecksums);
ACTION_F(n, n, n, n, addmd5sums);
ACTION_R(n, n, n, y, removereferences);
ACTION_R(n, n, n, y, removereference);
ACTION_R(n, n, n, n, dumpreferences);
ACTION_RF(n, n, n, n, dumpunreferenced);
ACTION_RF(n, n, n, n, deleteunreferenced);
ACTION_RF(n, n, n, y, deleteifunreferenced);
ACTION_R(n, n, n, y, addreference);
ACTION_R(n, n, n, y, addreferences);
ACTION_D(y, n, y, remove);
ACTION_D(n, n, y, removesrc);
ACTION_D(n, n, y, removesrcs);
ACTION_D(y, n, y, removefilter);
ACTION_D(y, n, y, removematched);
ACTION_B(y, n, y, buildneeded);
ACTION_B(y, n, y, list);
ACTION_B(y, n, y, ls);
ACTION_B(y, n, y, lsbycomponent);
ACTION_B(y, n, y, listfilter);
ACTION_B(y, n, y, listmatched);
ACTION_F(n, n, n, y, detect);
ACTION_F(n, n, n, y, forget);
ACTION_F(n, n, n, n, listmd5sums);
ACTION_F(n, n, n, n, listchecksums);
ACTION_B(n, n, n, dumpcontents);
ACTION_F(n, n, y, y, export);
ACTION_D(y, n, y, update);
ACTION_D(y, n, y, predelete);
ACTION_B(y, n, y, checkupdate);
ACTION_B(y, n, y, dumpupdate);
ACTION_L(n, n, n, n, cleanlists);
ACTION_D(y, n, y, pull);
ACTION_B(y, n, y, checkpull);
ACTION_B(y, n, y, dumppull);
ACTION_D(y, n, y, copy);
ACTION_D(y, n, y, copysrc);
ACTION_D(y, n, y, copyfilter);
ACTION_D(y, n, y, copymatched);
ACTION_D(y, n, y, restore);
ACTION_D(y, n, y, restoresrc);
ACTION_D(y, n, y, restorematched);
ACTION_D(y, n, y, restorefilter);
ACTION_D(y, n, y, addpackage);
ACTION_R(n, n, y, y, rereference);
ACTION_D(n, n, y, retrack);
ACTION_D(n, n, y, removetrack);
ACTION_D(n, n, y, removealltracks);
ACTION_D(n, n, y, tidytracks);
ACTION_B(n, n, y, dumptracks);
ACTION_RF(y, n, y, y, check);
ACTION_F(n, n, n, y, checkpool);
ACTION_F(n, n, n, n, collectnewchecksums);
ACTION_F(y, n, y, y, reoverride);
ACTION_F(y, n, y, y, repairdescriptions);
ACTION_F(y, n, y, y, redochecksums);
ACTION_RF(n, n, y, y, sizes);
ACTION_D(y, y, y, includedeb);
ACTION_D(y, y, y, includedsc);
ACTION_D(y, y, y, include);
ACTION_C(n, n, createsymlinks);
ACTION_C(n, n, checkuploaders);
ACTION_D(n, n, n, clearvanished);
ACTION_B(n, n, y, listdbidentifiers);
ACTION_C(n, n, listconfidentifiers);
ACTION_N(n, n, y, versioncompare);
ACTION_D(n, n, y, processincoming);
ACTION_R(n, n, y, y, gensnapshot);
ACTION_R(n, n, n, y, unreferencesnapshot);
ACTION_B(y, n, y, rerunnotifiers);
ACTION_D(y, n, y, flood);
ACTION_B(n, n, y, unusedsources);
ACTION_B(n, n, y, sourcemissing);
ACTION_B(n, n, y, reportcruft);

#endif
