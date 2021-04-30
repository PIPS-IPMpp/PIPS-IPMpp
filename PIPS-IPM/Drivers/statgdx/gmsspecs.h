#ifndef _P3___gmsspecs___H
#define _P3___gmsspecs___H

cnstdef {
GMSSPECS_maxnamelen = 63
};
cnstdef {
GMSSPECS_maxdim = 20
};
cnstdef {
GMSSPECS_maxnamelenv148 = 31
};
cnstdef {
GMSSPECS_maxdimv148 = 10
};
cnstdef {
GMSSPECS_bigindex = 10000000
};
cnstdef {
GMSSPECS_maxalgo = 150
};
cnstdef {
GMSSPECS_maxevalthreads = 64
};
typedef SYSTEM_uint8 GMSSPECS_tdimension;
typedef SYSTEM_uint8 GMSSPECS_tgdxdimension;
typedef SYSTEM_integer GMSSPECS_tindex[20];
typedef SYSTEM_shortstring GMSSPECS_tstrindex[20];
typedef GMSSPECS_tindex* GMSSPECS_ptindex;
extern SYSTEM_double GMSSPECS_old_valund;
extern SYSTEM_double GMSSPECS_old_valna;
extern SYSTEM_double GMSSPECS_old_valpin;
extern SYSTEM_double GMSSPECS_old_valmin;
extern SYSTEM_double GMSSPECS_old_valeps;
extern SYSTEM_double GMSSPECS_old_valacr;
extern SYSTEM_double GMSSPECS_valund;
extern SYSTEM_double GMSSPECS_valna;
extern SYSTEM_double GMSSPECS_valpin;
extern SYSTEM_double GMSSPECS_valmin;
extern SYSTEM_double GMSSPECS_valeps;
extern SYSTEM_double GMSSPECS_valacr;
extern SYSTEM_integer GMSSPECS_valnaint;
extern SYSTEM_double GMSSPECS_valiup;
extern SYSTEM_double GMSSPECS_valbig;
extern SYSTEM_double GMSSPECS_valsmall;
extern SYSTEM_double GMSSPECS_valtiny;
extern SYSTEM_double GMSSPECS_defiterlim;
typedef SYSTEM_byte GMSSPECS_tgdxspecialvalue; /* Anonymous */ enum {
   GMSSPECS_sv_valund, GMSSPECS_sv_valna, GMSSPECS_sv_valpin, GMSSPECS_sv_valmin, GMSSPECS_sv_valeps, GMSSPECS_sv_normal, GMSSPECS_sv_acronym
};
typedef SYSTEM_byte GMSSPECS_tgdxdatatype; /* Anonymous */ enum {
   GMSSPECS_dt_set, GMSSPECS_dt_par, GMSSPECS_dt_var, GMSSPECS_dt_equ, GMSSPECS_dt_alias
};
cnstdef {
GMSSPECS_gms_sv_valund = 0
};
cnstdef {
GMSSPECS_gms_sv_valna = 1
};
cnstdef {
GMSSPECS_gms_sv_valpin = 2
};
cnstdef {
GMSSPECS_gms_sv_valmin = 3
};
cnstdef {
GMSSPECS_gms_sv_valeps = 4
};
cnstdef {
GMSSPECS_gms_sv_normal = 5
};
cnstdef {
GMSSPECS_gms_sv_acronym = 6
};
cnstdef {
GMSSPECS_gms_dt_set = 0
};
cnstdef {
GMSSPECS_gms_dt_par = 1
};
cnstdef {
GMSSPECS_gms_dt_var = 2
};
cnstdef {
GMSSPECS_gms_dt_equ = 3
};
cnstdef {
GMSSPECS_gms_dt_alias = 4
};
typedef SYSTEM_byte GMSSPECS_tgmsvalue; /* Anonymous */ enum {
   GMSSPECS_xvreal, GMSSPECS_xvund, GMSSPECS_xvna, GMSSPECS_xvpin, GMSSPECS_xvmin, GMSSPECS_xveps, GMSSPECS_xvacr
};
typedef SYSTEM_byte GMSSPECS_txgmsvalue; /* Anonymous */ enum {
   GMSSPECS_vneg, GMSSPECS_vzero, GMSSPECS_vpos, GMSSPECS_vund, GMSSPECS_vna, GMSSPECS_vpin, GMSSPECS_vmin, GMSSPECS_veps, GMSSPECS_vacr
};

Function(GMSSPECS_tgmsvalue )
GMSSPECS_mapval(
      SYSTEM_double
x);

Function(GMSSPECS_txgmsvalue )
GMSSPECS_xmapval(
      SYSTEM_double
x);
typedef SYSTEM_byte GMSSPECS_tvarstyp; /* Anonymous */ enum {
   GMSSPECS_stypunknwn,
   GMSSPECS_stypbin,
   GMSSPECS_stypint,
   GMSSPECS_styppos,
   GMSSPECS_stypneg,
   GMSSPECS_stypfre,
   GMSSPECS_stypsos1,
   GMSSPECS_stypsos2,
   GMSSPECS_stypsemi,
   GMSSPECS_stypsemiint
};
typedef SYSTEM_byte GMSSPECS_tsetstyp; /* Anonymous */ enum { GMSSPECS_stypsetm, GMSSPECS_stypsets };
extern _P3SET_15 GMSSPECS_varstypx;
extern _P3SET_15 GMSSPECS_varstypi;
typedef _P3STR_15 _arr_0GMSSPECS[10];
extern _arr_0GMSSPECS GMSSPECS_varstyptxt;
typedef SYSTEM_byte GMSSPECS_tequstyp; /* Anonymous */ enum {
   GMSSPECS_styeque, GMSSPECS_styequg, GMSSPECS_styequl, GMSSPECS_styequn, GMSSPECS_styequx, GMSSPECS_styequc, GMSSPECS_styequb
};
typedef SYSTEM_integer _arr_1GMSSPECS[7];
extern _arr_1GMSSPECS GMSSPECS_equstypinfo;
typedef SYSTEM_byte GMSSPECS_tvarvaltype; /* Anonymous */ enum {
   GMSSPECS_vallevel, GMSSPECS_valmarginal, GMSSPECS_vallower, GMSSPECS_valupper, GMSSPECS_valscale
};
typedef SYSTEM_double GMSSPECS_tvarreca[5];
typedef _P3STR_7 _arr_2GMSSPECS[5];
extern _arr_2GMSSPECS GMSSPECS_sufftxt;
cnstdef {
GMSSPECS_nlconst_one = 1
};
cnstdef {
GMSSPECS_nlconst_ten = 2
};
cnstdef {
GMSSPECS_nlconst_tenth = 3
};
cnstdef {
GMSSPECS_nlconst_quarter = 4
};
cnstdef {
GMSSPECS_nlconst_half = 5
};
cnstdef {
GMSSPECS_nlconst_two = 6
};
cnstdef {
GMSSPECS_nlconst_four = 7
};
cnstdef {
GMSSPECS_nlconst_zero = 8
};
cnstdef {
GMSSPECS_nlconst_oosqrt2pi = 9
};
cnstdef {
GMSSPECS_nlconst_ooln10 = 10
};
cnstdef {
GMSSPECS_nlconst_ooln2 = 11
};
cnstdef {
GMSSPECS_nlconst_pi = 12
};
cnstdef {
GMSSPECS_nlconst_pihalf = 13
};
cnstdef {
GMSSPECS_nlconst_sqrt2 = 14
};
cnstdef {
GMSSPECS_nlconst_three = 15
};
cnstdef {
GMSSPECS_nlconst_five = 16
};
cnstdef {
GMSSPECS_equ_userinfo_base = 53
};
cnstdef {
GMSSPECS_equ_e = 0
};
cnstdef {
GMSSPECS_equ_g = 1
};
cnstdef {
GMSSPECS_equ_l = 2
};
cnstdef {
GMSSPECS_equ_n = 3
};
cnstdef {
GMSSPECS_equ_x = 4
};
cnstdef {
GMSSPECS_equ_c = 5
};
cnstdef {
GMSSPECS_equ_b = 6
};
typedef SYSTEM_uint8 _sub_4GMSSPECS;
typedef _P3STR_7 _arr_3GMSSPECS[7];
extern _arr_3GMSSPECS GMSSPECS_equstyp;
typedef SYSTEM_uint8 _sub_6GMSSPECS;
typedef SYSTEM_ansichar _arr_5GMSSPECS[7];
extern _arr_5GMSSPECS GMSSPECS_equctyp;
cnstdef {
GMSSPECS_var_x = 0
};
cnstdef {
GMSSPECS_var_b = 1
};
cnstdef {
GMSSPECS_var_i = 2
};
cnstdef {
GMSSPECS_var_s1 = 3
};
cnstdef {
GMSSPECS_var_s2 = 4
};
cnstdef {
GMSSPECS_var_sc = 5
};
cnstdef {
GMSSPECS_var_si = 6
};
typedef SYSTEM_uint8 _sub_8GMSSPECS;
typedef _P3STR_3 _arr_7GMSSPECS[7];
extern _arr_7GMSSPECS GMSSPECS_varstyp;
cnstdef {
GMSSPECS_var_x_f = 0
};
cnstdef {
GMSSPECS_var_x_n = 1
};
cnstdef {
GMSSPECS_var_x_p = 2
};
cnstdef {
GMSSPECS_bstat_lower = 0
};
cnstdef {
GMSSPECS_bstat_upper = 1
};
cnstdef {
GMSSPECS_bstat_basic = 2
};
cnstdef {
GMSSPECS_bstat_super = 3
};
cnstdef {
GMSSPECS_cstat_ok = 0
};
cnstdef {
GMSSPECS_cstat_nonopt = 1
};
cnstdef {
GMSSPECS_cstat_infeas = 2
};
cnstdef {
GMSSPECS_cstat_unbnd = 3
};
cnstdef {
GMSSPECS_gamsopt_cheat = 1
};
cnstdef {
GMSSPECS_gamsopt_cutoff = 2
};
cnstdef {
GMSSPECS_gamsopt_iterlim = 3
};
cnstdef {
GMSSPECS_gamsopt_nodlim = 4
};
cnstdef {
GMSSPECS_gamsopt_optca = 5
};
cnstdef {
GMSSPECS_gamsopt_optcr = 6
};
cnstdef {
GMSSPECS_gamsopt_reslim = 7
};
cnstdef {
GMSSPECS_gamsopt_reform = 8
};
cnstdef {
GMSSPECS_gamsopt_tryint = 9
};
cnstdef {
GMSSPECS_gamsopt_integer1 = 10
};
cnstdef {
GMSSPECS_gamsopt_integer2 = 11
};
cnstdef {
GMSSPECS_gamsopt_integer3 = 12
};
cnstdef {
GMSSPECS_gamsopt_integer4 = 13
};
cnstdef {
GMSSPECS_gamsopt_integer5 = 14
};
cnstdef {
GMSSPECS_gamsopt_real1 = 15
};
cnstdef {
GMSSPECS_gamsopt_real2 = 16
};
cnstdef {
GMSSPECS_gamsopt_real3 = 17
};
cnstdef {
GMSSPECS_gamsopt_real4 = 18
};
cnstdef {
GMSSPECS_gamsopt_real5 = 19
};
cnstdef {
GMSSPECS_gamsopt_workfactor = 20
};
cnstdef {
GMSSPECS_gamsopt_workspace = 21
};
cnstdef {
GMSSPECS_numsolm = 13
};
cnstdef {
GMSSPECS_nummodm = 19
};
cnstdef {
GMSSPECS_numsolprint = 5
};
cnstdef {
GMSSPECS_numhandlestat = 3
};
cnstdef {
GMSSPECS_numsolvelink = 7
};
cnstdef {
GMSSPECS_numsolveopt = 2
};
typedef SYSTEM_uint8 _sub_10GMSSPECS;
typedef _P3STR_15 _arr_9GMSSPECS[6];
extern _arr_9GMSSPECS GMSSPECS_solprinttxt;
typedef SYSTEM_uint8 _sub_12GMSSPECS;
typedef _P3STR_15 _arr_11GMSSPECS[4];
extern _arr_11GMSSPECS GMSSPECS_handlestattxt;
typedef SYSTEM_uint8 _sub_14GMSSPECS;
typedef _P3STR_31 _arr_13GMSSPECS[8];
extern _arr_13GMSSPECS GMSSPECS_solvelinktxt;
typedef SYSTEM_uint8 _sub_16GMSSPECS;
typedef _P3STR_15 _arr_15GMSSPECS[3];
extern _arr_15GMSSPECS GMSSPECS_solveopttxt;
typedef SYSTEM_uint8 _sub_18GMSSPECS;
typedef _P3STR_31 _arr_17GMSSPECS[13];
extern _arr_17GMSSPECS GMSSPECS_solvestatustxt;
typedef SYSTEM_uint8 _sub_20GMSSPECS;
typedef _P3STR_31 _arr_19GMSSPECS[19];
extern _arr_19GMSSPECS GMSSPECS_modelstatustxt;
typedef SYSTEM_uint8 _sub_22GMSSPECS;
typedef SYSTEM_integer _arr_21GMSSPECS[13];
extern _arr_21GMSSPECS GMSSPECS_solvetrigger;
typedef SYSTEM_uint8 _sub_24GMSSPECS;
typedef SYSTEM_integer _arr_23GMSSPECS[19];
extern _arr_23GMSSPECS GMSSPECS_modeltrigger;
typedef SYSTEM_uint8 _sub_26GMSSPECS;
typedef SYSTEM_integer _arr_25GMSSPECS[19];
extern _arr_25GMSSPECS GMSSPECS_modelsolution;
typedef struct GMSSPECS_tjacrec_S* GMSSPECS_tjac;
typedef struct GMSSPECS_tjacrec_S {
   SYSTEM_double coef;
   GMSSPECS_tjac nextrow;
   GMSSPECS_tjac nextnlrow;
   GMSSPECS_tjac nextcol;
   SYSTEM_longint row;
   SYSTEM_longint col;
   SYSTEM_boolean nl;
} GMSSPECS_tjacrec;

typedef SYSTEM_uint32 _sub_27GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tjacptr[10000000];
typedef GMSSPECS_tjacptr* GMSSPECS_tptrjacptr;
typedef SYSTEM_byte GMSSPECS_bndset; /* Anonymous */ enum {
   GMSSPECS_bndf, GMSSPECS_bndb, GMSSPECS_bndl, GMSSPECS_bndu, GMSSPECS_bnde
};
typedef SYSTEM_uint32 _sub_28GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequtype[10000000];
typedef GMSSPECS_tequtype* GMSSPECS_tptrequtype;
typedef SYSTEM_uint32 _sub_29GMSSPECS;
typedef SYSTEM_double GMSSPECS_trhs[10000000];
typedef GMSSPECS_trhs* GMSSPECS_tptrrhs;
typedef SYSTEM_uint32 _sub_30GMSSPECS;
typedef SYSTEM_double GMSSPECS_tequm[10000000];
typedef GMSSPECS_tequm* GMSSPECS_tptrequm;
typedef SYSTEM_uint32 _sub_31GMSSPECS;
typedef SYSTEM_boolean GMSSPECS_tequbas[10000000];
typedef GMSSPECS_tequbas* GMSSPECS_tptrequbas;
typedef SYSTEM_uint32 _sub_32GMSSPECS;
typedef SYSTEM_double GMSSPECS_tequl[10000000];
typedef GMSSPECS_tequl* GMSSPECS_tptrequl;
typedef SYSTEM_uint32 _sub_33GMSSPECS;
typedef SYSTEM_integer GMSSPECS_tequstat[10000000];
typedef GMSSPECS_tequstat* GMSSPECS_tptrequstat;
typedef SYSTEM_uint32 _sub_34GMSSPECS;
typedef SYSTEM_integer GMSSPECS_tequcstat[10000000];
typedef GMSSPECS_tequstat* GMSSPECS_tptrequctat;
typedef SYSTEM_uint32 _sub_35GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequmatch[10000000];
typedef GMSSPECS_tequmatch* GMSSPECS_tptrequmatch;
typedef SYSTEM_uint32 _sub_36GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tnlstart[10000000];
typedef GMSSPECS_tnlstart* GMSSPECS_tptrnlstart;
typedef SYSTEM_uint32 _sub_37GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tnlend[10000000];
typedef GMSSPECS_tnlend* GMSSPECS_tptrnlend;
typedef SYSTEM_uint32 _sub_38GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tnlfstart[10000000];
typedef GMSSPECS_tnlfstart* GMSSPECS_tptrnlfstart;
typedef SYSTEM_uint32 _sub_39GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tnlfend[10000000];
typedef GMSSPECS_tnlfend* GMSSPECS_tptrnlfend;
typedef SYSTEM_uint32 _sub_40GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequnz[10000000];
typedef GMSSPECS_tequnz* GMSSPECS_tptrequnz;
typedef SYSTEM_uint32 _sub_41GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequnlnz[10000000];
typedef GMSSPECS_tequnlnz* GMSSPECS_tptrequnlnz;
typedef SYSTEM_uint32 _sub_42GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequnextnl[10000000];
typedef GMSSPECS_tequnextnl* GMSSPECS_tptrequnextnl;
typedef SYSTEM_uint32 _sub_43GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tequfirst[10000000];
typedef GMSSPECS_tequfirst* GMSSPECS_tptrequfirst;
typedef SYSTEM_uint32 _sub_44GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tequfirstnl[10000000];
typedef GMSSPECS_tequfirstnl* GMSSPECS_tptrequfirstnl;
typedef SYSTEM_uint32 _sub_45GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tequlast[10000000];
typedef GMSSPECS_tequlast* GMSSPECS_tptrequlast;
typedef SYSTEM_uint32 _sub_46GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tequlastnl[10000000];
typedef GMSSPECS_tequlastnl* GMSSPECS_tptrequlastnl;
typedef SYSTEM_uint32 _sub_47GMSSPECS;
typedef SYSTEM_double GMSSPECS_tequscale[10000000];
typedef GMSSPECS_tequscale* GMSSPECS_tptrequscale;
typedef SYSTEM_uint32 _sub_48GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tequperm[10000000];
typedef GMSSPECS_tequperm* GMSSPECS_tptrequperm;
typedef SYSTEM_uint32 _sub_49GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvartype[10000000];
typedef GMSSPECS_tvartype* GMSSPECS_tptrvartype;
typedef SYSTEM_uint32 _sub_50GMSSPECS;
typedef SYSTEM_double GMSSPECS_tvarlo[10000000];
typedef GMSSPECS_tvarlo* GMSSPECS_tptrvarlo;
typedef SYSTEM_uint32 _sub_51GMSSPECS;
typedef SYSTEM_double GMSSPECS_tvarl[10000000];
typedef GMSSPECS_tvarl* GMSSPECS_tptrvarl;
typedef SYSTEM_uint32 _sub_52GMSSPECS;
typedef SYSTEM_double GMSSPECS_tvarup[10000000];
typedef GMSSPECS_tvarup* GMSSPECS_tptrvarup;
typedef SYSTEM_uint32 _sub_53GMSSPECS;
typedef SYSTEM_double GMSSPECS_tvarm[10000000];
typedef GMSSPECS_tvarm* GMSSPECS_tptrvarm;
typedef SYSTEM_uint32 _sub_54GMSSPECS;
typedef SYSTEM_boolean GMSSPECS_tvarbas[10000000];
typedef GMSSPECS_tvarbas* GMSSPECS_tptrvarbas;
typedef SYSTEM_uint32 _sub_55GMSSPECS;
typedef GMSSPECS_bndset GMSSPECS_tvarbnd[10000000];
typedef GMSSPECS_tvarbnd* GMSSPECS_tptrvarbnd;
typedef SYSTEM_uint32 _sub_56GMSSPECS;
typedef SYSTEM_integer GMSSPECS_tvarstat[10000000];
typedef GMSSPECS_tvarstat* GMSSPECS_tptrvarstat;
typedef SYSTEM_uint32 _sub_57GMSSPECS;
typedef SYSTEM_integer GMSSPECS_tvarcstat[10000000];
typedef GMSSPECS_tvarstat* GMSSPECS_tptrvarcstat;
typedef SYSTEM_uint32 _sub_58GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarsos[10000000];
typedef GMSSPECS_tvarsos* GMSSPECS_tptrvarsos;
typedef SYSTEM_uint32 _sub_59GMSSPECS;
typedef SYSTEM_double GMSSPECS_tprior[10000000];
typedef GMSSPECS_tprior* GMSSPECS_tptrprior;
typedef SYSTEM_uint32 _sub_60GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tvarfirst[10000000];
typedef GMSSPECS_tvarfirst* GMSSPECS_tptrvarfirst;
typedef SYSTEM_uint32 _sub_61GMSSPECS;
typedef GMSSPECS_tjac GMSSPECS_tvarlast[10000000];
typedef GMSSPECS_tvarlast* GMSSPECS_tptrvarlast;
typedef SYSTEM_uint32 _sub_62GMSSPECS;
typedef SYSTEM_double GMSSPECS_tvarscale[10000000];
typedef GMSSPECS_tvarscale* GMSSPECS_tptrvarscale;
typedef SYSTEM_uint32 _sub_63GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarperm[10000000];
typedef GMSSPECS_tvarperm* GMSSPECS_tptrvarperm;
typedef SYSTEM_uint32 _sub_64GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarnz[10000000];
typedef GMSSPECS_tvarnz* GMSSPECS_tptrvarnz;
typedef SYSTEM_uint32 _sub_65GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarnlnz[10000000];
typedef GMSSPECS_tvarnlnz* GMSSPECS_tptrvarnlnz;
typedef SYSTEM_uint32 _sub_66GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarmatch[10000000];
typedef GMSSPECS_tvarmatch* GMSSPECS_tptrvarmatch;
typedef SYSTEM_uint32 _sub_67GMSSPECS;
typedef SYSTEM_longint GMSSPECS_tvarcluster[10000000];
typedef GMSSPECS_tvarcluster* GMSSPECS_tptrvarcluster;

Function(SYSTEM_double )
GMSSPECS_old_new_val(
      SYSTEM_double
x);

extern void _Init_Module_gmsspecs(void);
extern void _Final_Module_gmsspecs(void);

#endif /* ! defined _P3___gmsspecs___H */
