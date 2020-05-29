import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import make_interp_spline, BSpline


def plot_results(x, y, x_label, y_label, axis, graph_label, result_name):
    plt.rc('grid', linestyle="dotted", color='grey')
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.axis(axis)
    plt.yticks(np.arange(0, axis[3] + 1, 1))
    plt.xticks(x)
    plt.plot(x, y, label=graph_label)

    plt.plot(x, x)

    plt.legend()
    plt.grid()
    plt.savefig("".join(["images/", result_name]))
    plt.close()


if __name__ == "__main__":
    files = "C:\\Users\\luigi\\Google Drive\\Materiale\\PCPC\\Progetto\\Risultati.txt"
    weak_scaling = []
    weak_scaling_efficiency = []
    strong_scaling = []
    strong_scaling_efficiency = []
    strong_scaling_speedup = []
    flag = 0
    i = 1
    with open(files, "r") as fp:
        line = fp.readline().strip()
        line = fp.readline().strip()
        while line:
            if(line == "TEST STRONG"):
                flag = 1
                i = i - 16
                line = fp.readline().strip()
                continue
            if(len(line.split("time:")) > 1):
                if(flag == 0):
                    weak_scaling.append(
                        float(line.split("time:")[1].strip()) / 60.0)
                    weak_scaling_efficiency.append(
                        (weak_scaling[0] / (weak_scaling[i-1])))
                else:
                    strong_scaling.append(
                        float(line.split("time:")[1].strip()) / 60.0)
                    strong_scaling_speedup.append(
                        (strong_scaling[0] / strong_scaling[i-1]))
                    strong_scaling_efficiency.append(
                        (strong_scaling[0] / (strong_scaling[i-1] * i)))

            line = fp.readline().strip()
            i = i + 1

    # General configuration
    x = np.linspace(1, 16, 16)
    # Strong Scaling
    plt.rc('grid', linestyle="dotted", color='grey')
    plt.xlabel("Num nodes")
    plt.ylabel("Time (minutes)")
    plt.axis([1, 16, 1, 16])
    plt.yticks(np.arange(0, 17, 1))
    plt.xticks(x)
    plt.plot(x, strong_scaling, label="Strong Scaling")
    # Best results possible
    best_result = []
    count = 1.0
    for i in strong_scaling:
        best_result.append(strong_scaling[0] / count)
        count = count + 1
    xnew = np.linspace(1,16,600)
    spl = make_interp_spline(x, best_result, k=3)  # type: BSpline
    power_smooth = spl(xnew)
    plt.plot(xnew, power_smooth, label="Best result possible")

    plt.legend()
    plt.grid()
    plt.savefig("".join(["images/", "strong_scaling.png"]))
    plt.close()

    # Strong Scaling Speedup
    plt.rc('grid', linestyle="dotted", color='grey')
    plt.xlabel("Num nodes")
    plt.ylabel("Speedup")
    plt.axis([1, 16, 1, 16])
    plt.yticks(np.arange(0, 17, 1))
    plt.xticks(x)
    plt.plot(x, strong_scaling_speedup, label="Strong Scaling Speedup")
    # Best results possible
    plt.plot(x, x, label="Best result possible")

    plt.legend()
    plt.grid()
    plt.savefig("".join(["images/", "strong_scaling_speedup.png"]))
    plt.close()

    # Strong Scaling efficiency
    plt.rc('grid', linestyle="dotted", color='grey')
    plt.xlabel("Num nodes")
    plt.ylabel("Efficiency")
    plt.axis([1, 16, 0, 1.5])
    plt.xticks(x)
    plt.yticks(np.arange(0, 1.5, step=0.1))
    plt.plot(x, strong_scaling_efficiency, label="Strong Scaling Efficiency")
    plt.plot(x, np.ones(16), label="Best result possible")
    plt.legend()
    plt.grid()
    plt.savefig("".join(["images/", "strong_scaling_eff.png"]))
    plt.close()

    # Weak Scaling
    plt.rc('grid', linestyle="dotted", color='grey')
    plt.xlabel("Num nodes")
    plt.ylabel("Efficiency")
    plt.axis([1, 16, 0, 1.5])
    plt.xticks(x)
    plt.yticks(np.arange(0, 1.5, step=0.1))
    plt.plot(x, weak_scaling_efficiency, label="Weak Scaling Efficiency")
    plt.plot(x, np.ones(16), label="Best result possible")
    plt.legend()
    plt.grid()
    plt.savefig("".join(["images/", "weak_scaling_eff.png"]))
    plt.close()
