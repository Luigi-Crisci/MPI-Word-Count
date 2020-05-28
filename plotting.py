import matplotlib.pyplot as plt
import numpy as np


def plot_results(x, y, x_label, y_label, axis, graph_label, result_name):
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.axis(axis)
    plt.plot(x, y, label=graph_label)
    plt.legend()
    plt.savefig("".join(["images/", result_name]))
    plt.close()


if __name__ == "__main__":
    files = "C:\\Users\\luigi\\Google Drive\\Materiale\\PCPC\\Progetto\\Risultati.txt"
    weak_scaling = []
    weak_scaling_efficiency = []
    strong_scaling = []
    strong_scaling_efficiency = []
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
                    strong_scaling_efficiency.append(
                        (strong_scaling[0] / (strong_scaling[i-1] * i)))

            line = fp.readline().strip()
            i = i + 1

    # General configuration
    x = np.linspace(1, 16, 16)
    # Strong Scaling
    plot_results(x, strong_scaling, "Num nodes", "Time (minutes)", [
                 1, 16, 0, 15], "Strong Scaling", "strong_scaling.png")
    # Strong Scaling efficiency
    plot_results(x, strong_scaling_efficiency, "Num nodes", "Efficiency", [
                 1, 16, 0, 1.5], "Strong Scaling Efficiency", "strong_scaling_eff.png")
    # Weak Scaling
    plot_results(x, weak_scaling_efficiency, "Num nodes", "Efficiency", [
                 1, 16, 0, 1.5], "Weak Scaling Efficiency", "weak_scaling_eff.png")
