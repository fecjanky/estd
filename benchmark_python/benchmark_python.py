#!/usr/bin/env python
import os
import argparse
import subprocess
import re
import io
import numpy as np
import matplotlib.pyplot as plt

def benchmark_wiht_params(bin,num_elems,num_iterations,type):
    with subprocess.Popen([bin,str(num_elems),str(num_iterations),type],stdout=subprocess.PIPE) as p:
        for l in io.TextIOWrapper(p.stdout):
            match = re.match(r"(\d+)\s*[mun]?s\s*,",l,re.IGNORECASE)
            if match is None : raise ValueError
            return int(float(match.group(1)))

try:
    parser = argparse.ArgumentParser()
    parser.add_argument("-b","--benchmarkexe",type=str,help="location to benchmark executable")
    parser.add_argument("-l","--lower",type=int,help="low mark for elem num",default=1000)
    parser.add_argument("-u","--upper",type=int,help="high mark for elem num",default=100000)
    parser.add_argument("-s","--step",type=int,help="step for elem num",default=1000)
    parser.add_argument("-c","--count",type=int,help="count of repetition",default=16)
    parser.add_argument("-i","--iteration",type=int,help="count of iteration",default=16)
    args = parser.parse_args()
    test_vec = [(x,args.iteration) for x in range(args.lower,args.upper,args.step)]
    poly_res = []
    unique_res = []
    for t in test_vec:
        res_poly_vec = [benchmark_wiht_params(args.benchmarkexe,t[0],t[1],"poly_vec") for x in range(0,args.count)]
        poly_res.append((t,np.mean(res_poly_vec)))
        res_unique_vec = [benchmark_wiht_params(args.benchmarkexe,t[0],t[1],"unique_ptr_vec") for x in range(0,args.count)] 
        unique_res.append((t,np.mean(res_unique_vec)))

    t_poly = [x[0][0] for x in poly_res] 
    poly_data = [x[1] for x in poly_res]
    t_non_poly = [x[0][0] for x in unique_res]  
    non_poly_data =[x[1] for x in unique_res]
    plt.plot(t_poly,poly_data,'g^',t_non_poly,non_poly_data,'r--')
    plt.show()
except Exception as e:
    print(e)
    parser.print_help()



