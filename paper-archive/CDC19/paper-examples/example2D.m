%% 2D Example of "Computing controlled invariant sets in two moves"
%
%% Authors: Tzanis Anevlavis, Paulo Tabuada
% Copyright (C) 2019, Tzanis Anevlavis, Paulo Tabuada
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful, but 
% WITHOUT ANY WARRANTY; without even the implied warranty of 
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program. If not, see <http://www.gnu.org/licenses/>.
%
%
% This code is part of the implementation of the algorithm proposed in:
% Tzanis Anevlavis and Paulo Tabuada, "Computing controlled invariant sets
% in two moves", in 2019 IEEE Conference on Decision and Control, 
% and is publicly available at: https://github.com/janis10/cis2m
%
% For any comments contact Tzanis Anevlavis @ janis10@ucla.edu.
%
%
%
%
%% Description:
% This script generates an example in \R^2, in which there also constraints
% on the input u. 
%
% Original system: x+ = Ao x + Bo
% State constraints: Go x <= Fo
% Input constraints: u \in [-umin,umax].
%
% This function makes use of the Multi-Parametric Toolbox 3.0:
% M. Herceg, M. Kvasnica, C. Jones, and M. Morari, 
% ``Multi-Parametric Toolbox 3.0,'' in Proc. of the European Control 
% Conference, Z�rich, Switzerland, July 17-19 2013, pp. 502-510, 
% http://control.ee.ethz.ch/mpt.

%% Exammple Setup:
close all
clear
clc
% Relative to this file:
addpath('../../legacy_code/');

% Original system:
A = [1.5 1; 0 1]; B = [0.5; 0.25];
% State constraints:
Gx =[   0.9147   -0.5402;
        0.2005    0.6213;
       -0.8193    0.9769;
       -0.4895   -0.8200;
        0.7171   -0.3581;
        0.8221    0.0228;
        0.3993   -0.8788];
Fx = [  0.5566;
        0.8300;
        0.7890;
        0.3178;
        0.4522;
        0.7522;
        0.1099];
D = Polyhedron('H',[G F]);
% Input constraints:
umin = -2;
umax = 2;
Gu = [1; -1]; Fu = [umax; -umin];
U = Polyhedron('H',[Gu Fu]);

%% Compute controlled invariant set in two moves:
method = 'CDC19';
cisMat = computeCIS(A,B,Gx,Fx,Gu,Fu,[],[],[],method);
cis = Polyhedron('H',cisMat);

%% Compute MCIS using MPT3: 
system = LTISystem('A',A,'B',B);
mcis = system.invariantSet('X',D,'U',U,'maxIterations',300);

%% Plotting:
figure; plot(D, 'color', 'blue', mcis, 'color', 'lightgray', cis, 'color', 'white')