* ./gamsexample.sh -NP=4 -BLOCKS=5 -GAMSFILE=./hier_approach_4blocks_2by2

Set i rows    / i1*i20 /
    j columns / j1*j13 /;

parameter g(j) obj coefficients / j1 -1, j2 -1, j3 -1, j4 1, j5 1, j6 1, j7 1, j8 1, j9 1, j10 1, j11 1, j12 1, j13 1 /
          bA(i) right hand side  / i1 4, i2 8, i3 4, i4 13, i5 4, i6 6, i7 4, i8 13, i9 4, i10 13, i11 3, i12 3, i13 3, i14 3, i15 3, i16 10, i17 3, i18 10, i19 0, i20 7 /
*          clow(i) c left hand side   / i1 0, i2 1, i3 -1, i4 -100, i5 -100, i6 -100, i7 -100, i8 -100, i9 -100, i10 -100, i11 1, i12 -100, i13 -100, i14 -100, i15 -100, i16 -100, i17 -100, i18 -100, i19 -100/
          cupp(i) c right hand side   / i1 1, i2 8, i3 2, i4 9, i5 10, i6 8, i7 10, i8 10, i9 2, i10 9, i11 100, i12 4, i13 10, i14 50, i15 13, i16 113, i17 1, i18 4, i19 10, i20 1000 /


* in this example the singletonColumnPresolver should substitute j1 (free) and put it into the objective
Table A(i,j)
    j1    j2    j3    j4    j5    j6    j7    j8    j9   j10   j11   j12   j13
i1   1           1     2     1
i2               1    -2     4
i3                                 2     1
i4         1                      -2     4
i5                                             2     1
i6                                            -2     4
i7                                                         2     1
i8         1                                              -2     4                  
i9                                                                     2     1
i10        1                                                          -2     4
i11                          1     1
i12                          2    -1
i13                                      1     1
i14                                      2    -1
i15                                                  1     1
i16        1                                         2    -1
i17                                                              1     1
i18        1                                                     2    -1
i19       -1           1    -1           1           1           1           1
i20                    1                 1                       1           1
; 
*   -2     7     2     1     2     1     2     1     2     1     2     1     2
* expected values for x full determined by Ax=b
* objective = 2 - 7 - 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 2 = 8 
Table C(i,j)
    j1    j2    j3    j4    j5    j6    j7    j8    j9   j10   j11   j12   j13   
i1   1                 1     1
i2   1                 2     4
i3   1                             1     1
i4   1                             2     4
i5   1                                         1     1
i6   1                                         2     4
i7   1                                                     1     1
i8   1                                                     2     4
i9   1                                                                 1     1
i10  1                                                                 2     4
i11  1                       1     1
i12  1                       1     4
i13  1                                   1     1     
i14  1                                   1     4
i15  1                                               1     1                                          
i16  1                                               1     4
i17  1                                                           1     1
i18  1                                                           1     4
i19  1                                         1           1     1
i20  1     1     1     1     1     1     1     1     1     1     1     1     1
;

Variables          x(j) * / j4.lo -5 /
Variable           z      objective variable
Equations          e(i)   equality equations
*                   ge(i)  greater than inequality equations
                   le(i)  less than inequality equations
                   defobj objective function;

defobj.. z =e= sum(j, g(j)*x(j));
e(i)..   sum(j, A(i,j)*x(j)) =e= bA(i);
*ge(i)..  sum(j, C(i,j)*x(j)) =g= clow(i);
le(i)..  sum(j, C(i,j)*x(j)) =l= cupp(i);

Model m /all/ ;


$ifthen %METHOD%==PIPS
*annotations for variables:
  x.stage('j6') = 2;
  x.stage('j7') = 2;
  x.stage('j8') = 3;
  x.stage('j9') = 3;
  x.stage('j10') = 4;
  x.stage('j11') = 4;
  x.stage('j12') = 5;
  x.stage('j13') = 5;
*annotations for equations:
  e.stage('i1') = 1;
  e.stage('i2') = 1;
  e.stage('i3') = 2;
  e.stage('i4') = 2;
  e.stage('i5') = 3;
  e.stage('i6') = 3;
  e.stage('i7') = 4;
  e.stage('i8') = 4;
  e.stage('i9') = 5;
  e.stage('i10') = 5;
  e.stage('i11') = 6;
  e.stage('i12') = 6;
  e.stage('i13') = 6;
  e.stage('i14') = 6;
  e.stage('i15') = 6;
  e.stage('i16') = 6;
  e.stage('i17') = 6;
  e.stage('i18') = 6;
  e.stage('i19') = 6;
  e.stage('i20') = 6;
*  ge.stage('i1') = 1;
*  ge.stage('i2') = 1;
*  ge.stage('i3') = 1;
*  ge.stage('i4') = 2;
*  ge.stage('i5') = 2;
*  ge.stage('i6') = 2;
*  ge.stage('i7') = 3;
*  ge.stage('i8') = 3;
*  ge.stage('i9') = 3;
*  ge.stage('i10') = 4;
*  ge.stage('i11') = 4;
*  ge.stage('i12') = 4;
*  ge.stage('i13') = 5;
*  ge.stage('i14') = 5;
  le.stage('i1') = 1;
  le.stage('i2') = 1;
  le.stage('i3') = 2;
  le.stage('i4') = 2;
  le.stage('i5') = 3;
  le.stage('i6') = 3;
  le.stage('i7') = 4;
  le.stage('i8') = 4;
  le.stage('i9') = 5;
  le.stage('i10') = 5;
  le.stage('i11') = 6;
  le.stage('i12') = 6;
  le.stage('i13') = 6;
  le.stage('i14') = 6;
  le.stage('i15') = 6;
  le.stage('i16') = 6;
  le.stage('i17') = 6;
  le.stage('i18') = 6;
  le.stage('i19') = 6;
  le.stage('i20') = 6;
  defobj.stage  = 6;


* For creation of gdx files:
$ echo jacobian hier_approach_4blocks_2by2.gdx > convertd.opt
  option lp=convertd;
  m.optfile = 1;
  solve m use lp min z;
$else
  option lp=cplex;
$ onecho > cplex.opt
  lpmethod 4
  solutiontype 2
$ offecho
  m.optfile = 1;
  solve m use lp min z;
$endif
