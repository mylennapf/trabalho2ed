import matplotlib.pyplot as plt
taxas = [10, 20, 30, 40, 50, 60, 70, 80, 90, 99]
tempos_simples = [0.000002, 0.000001, 0.000002, 0.000001, 0.000002, 0.000002, 0.000001, 0.000001, 0.000001, 0.000002]  # Substitua pelos seus valores
tempos_dupla = [0.000002, 0.000001, 0.000002, 0.000000, 0.000001, 0.000002, 0.000001, 0.000001, 0.000001, 0.000002]  # Substitua pelos seus valores
plt.plot(taxas, tempos_simples, label="Hash Simples")
plt.plot(taxas, tempos_dupla, label="Hash Dupla")
plt.xlabel("Taxa de Ocupação (%)")
plt.ylabel("Tempo de Busca (s)")
plt.legend()
plt.show()