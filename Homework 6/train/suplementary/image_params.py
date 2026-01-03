import matplotlib as mpl

lc = mpl.colors.ListedColormap
cm = mpl.cm

#def set_imageFontSize(titleSize = 20, labelSize = 16, tickSize = 14):
def set_imageFontSize(titleSize = 16, labelSize = 14, tickSize = 12):
    mpl.rcParams["axes.titlesize"] = titleSize
    mpl.rcParams["axes.labelsize"] = labelSize
    mpl.rcParams['xtick.labelsize'] = tickSize
    mpl.rcParams['ytick.labelsize'] = tickSize