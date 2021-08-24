import matplotlib.pyplot as plt






def bar(hwpl,num):

    import numpy as np
    sum = 0
    for n in num[100:]:
        sum+=n

    num = np.array(num[100:])/sum * 100
    

    fig,ax = plt.subplots()

    x = np.arange(len(num))
    ax.bar(x,num)
    # plt.xticks(x,hwpl)
    
    # ax.set_yscale('log')
    
    plt.show()



def plts(hwpl,num,filename):
    import numpy as np
    fig,ax = plt.subplots()
    x = np.arange(len(num))
    hwpl = np.array(hwpl)

    accum = []
    acc = 0
    for n in num:
        acc+=n
        accum.append(acc)

    accum = np.array(accum)/acc * 100

    ax.plot(hwpl,accum)
    
    ax.set_ylabel("accumulation(%)")
    ax.set_xlabel("HWPL")
    ax.set_title(filename)
    plt.show()

def plotFile(filename,callback):
    
    hwpls = []
    nums = []
    with open(filename+"_hpwl.txt",'r') as f:
        lines = f.readlines()
        for line in lines[2:]:
            hwpl,num = line.split('|')
            hwpls.append(int(hwpl))
            nums.append(int(num))
    
  
    callback(hwpls,nums,filename)


if __name__ == '__main__':

    plotFile("case5",plts)

