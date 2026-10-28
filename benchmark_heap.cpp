#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Les trois classes ci-dessous exposent le meme tableau indexe, avec
// des couts d'acces differents selon le conteneur utilise.
class VectorStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    int& at(std::size_t index) { return data.at(index); }
    const int& at(std::size_t index) const { return data.at(index); }
    void pushBack(int value) { data.push_back(value); }
    void popBack() { data.pop_back(); }
    void clear() { data.clear(); }

private:
    std::vector<int> data;
};

class ListStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }

    int& at(std::size_t index) {
        return *std::next(data.begin(), static_cast<std::ptrdiff_t>(index));
    }

    const int& at(std::size_t index) const {
        return *std::next(data.begin(), static_cast<std::ptrdiff_t>(index));
    }

    void pushBack(int value) { data.push_back(value); }
    void popBack() { data.pop_back(); }
    void clear() { data.clear(); }

private:
    std::list<int> data;
};

class MapStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    int& at(std::size_t index) { return data.at(index); }
    const int& at(std::size_t index) const { return data.at(index); }

    void pushBack(int value) {
        data.emplace(data.size(), value);
    }

    void popBack() {
        data.erase(data.size() - 1);
    }

    void clear() { data.clear(); }

private:
    std::map<std::size_t, int> data;
};

// Tas min generique : la racine (indice 0) contient toujours le minimum.
template <typename Store>
class MinHeap {
public:
    std::size_t size() const { return values.size(); }
    bool empty() const { return values.empty(); }

    void insert(int value) {
        values.pushBack(value);
        siftUp(values.size() - 1);
    }

    int minimum() const {
        if (empty()) {
            throw std::runtime_error("Le tas est vide.");
        }
        return values.at(0);
    }

    bool contains(int value) const {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values.at(i) == value) {
                return true;
            }
        }
        return false;
    }

    bool erase(int value) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values.at(i) == value) {
                eraseAt(i);
                return true;
            }
        }
        return false;
    }

    int removeMinimum() {
        const int result = minimum();
        eraseAt(0);
        return result;
    }

    std::string toString() const {
        std::ostringstream output;
        output << "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                output << ", ";
            }
            output << values.at(i);
        }
        output << "]";
        return output.str();
    }

private:
    Store values;

    void siftUp(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (values.at(parent) <= values.at(index)) {
                break;
            }
            std::swap(values.at(parent), values.at(index));
            index = parent;
        }
    }

    void siftDown(std::size_t index) {
        while (index < values.size()) {
            const std::size_t left = 2 * index + 1;
            const std::size_t right = left + 1;
            std::size_t smallest = index;

            if (left < values.size() && values.at(left) < values.at(smallest)) {
                smallest = left;
            }
            if (right < values.size() && values.at(right) < values.at(smallest)) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }
            std::swap(values.at(index), values.at(smallest));
            index = smallest;
        }
    }

    void eraseAt(std::size_t index) {
        const std::size_t last = values.size() - 1;
        if (index != last) {
            values.at(index) = values.at(last);
        }
        values.popBack();

        if (index < values.size()) {
            if (index > 0 && values.at(index) < values.at((index - 1) / 2)) {
                siftUp(index);
            } else {
                siftDown(index);
            }
        }
    }
};

using HeapVector = MinHeap<VectorStore>;
using HeapList = MinHeap<ListStore>;
using HeapMap = MinHeap<MapStore>;

struct BenchmarkResult {
    std::string containerName;
    std::string heapSnapshot;
    double insertionMs = 0.0;
    double searchMs = 0.0;
    double deletionMs = 0.0;
    double displayMs = 0.0;
    std::size_t queries = 0;
    std::size_t found = 0;
};

template <typename Function>
double measureMilliseconds(Function function) {
    const auto start = std::chrono::steady_clock::now();
    function();
    const auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

template <typename Store>
BenchmarkResult runBenchmark(
    const std::string& containerName,
    const std::vector<int>& inputValues,
    const std::vector<int>& queries) {
    MinHeap<Store> heap;
    BenchmarkResult result;
    result.containerName = containerName;
    result.queries = queries.size();

    result.insertionMs = measureMilliseconds([&]() {
        for (int value : inputValues) {
            heap.insert(value);
        }
    });

    result.searchMs = measureMilliseconds([&]() {
        for (int value : queries) {
            if (heap.contains(value)) {
                ++result.found;
            }
        }
    });

    result.displayMs = measureMilliseconds([&]() {
        result.heapSnapshot = heap.toString();
    });

    result.deletionMs = measureMilliseconds([&]() {
        while (!heap.empty()) {
            heap.removeMinimum();
        }
    });

    return result;
}

void writeHeapLine(std::ofstream& output, const std::string& name, const std::string& heap) {
    output << name << " : " << heap << '\n';
}

template <typename Store>
void writeInteractiveHeap(std::ofstream& output, const std::string& name, const MinHeap<Store>& heap) {
    writeHeapLine(output, name, heap.toString());
}

void saveInteractiveHeaps(
    const HeapVector& vectorHeap,
    const HeapList& listHeap,
    const HeapMap& mapHeap) {
    std::ofstream output("output.txt", std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Impossible d'ouvrir output.txt en ecriture.");
    }

    output << "Affichage des tas min (ordre interne du tableau du tas)\n\n";
    writeInteractiveHeap(output, "vector", vectorHeap);
    writeInteractiveHeap(output, "list", listHeap);
    writeInteractiveHeap(output, "map", mapHeap);
    output << "\nLa racine de chaque tas est son element minimum.\n";
}

void saveBenchmarkResults(
    std::size_t count,
    const std::vector<BenchmarkResult>& results) {
    std::ofstream output("output.txt", std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Impossible d'ouvrir output.txt en ecriture.");
    }

    output << "BENCHMARK DES TAS MIN\n"
           << "Nombre d'elements inseres : " << count << "\n"
           << "Les trois structures utilisent les memes valeurs aleatoires.\n"
           << "Les durees mesurees n'incluent pas l'ecriture de ce fichier.\n\n";

    for (const BenchmarkResult& result : results) {
        output << "=== Conteneur : " << result.containerName << " ===\n"
               << "Tas apres insertion (avant suppressions) : "
               << result.heapSnapshot << '\n'
               << "Minimum : ";
        if (count == 0) {
            output << "aucun (tas vide)\n";
        } else {
            const std::string& heap = result.heapSnapshot;
            const std::size_t firstValue = heap.find_first_not_of("[] ");
            const std::size_t endValue = heap.find_first_of(",]", firstValue);
            output << heap.substr(firstValue, endValue - firstValue) << '\n';
        }

        output << std::fixed << std::setprecision(3)
               << "Insertion (" << count << " elements) : " << result.insertionMs << " ms\n"
               << "Recherche (" << result.queries << " requetes, "
               << result.found << " trouvees) : " << result.searchMs << " ms\n"
               << "Suppression de tous les elements par minimum : "
               << result.deletionMs << " ms\n"
               << "Preparation de l'affichage : " << result.displayMs << " ms\n\n";
    }
}

int readInteger(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        int value = 0;
        if (std::cin >> value) {
            return value;
        }
        if (std::cin.eof()) {
            throw std::runtime_error("Fin de la saisie.");
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Saisie invalide : entrez un nombre entier.\n";
    }
}

void runInteractiveMenu() {
    HeapVector vectorHeap;
    HeapList listHeap;
    HeapMap mapHeap;

    while (true) {
        std::cout << "\n===== MENU - TAS MIN =====\n"
                  << "1. Inserer une valeur dans les trois tas\n"
                  << "2. Rechercher une valeur dans les trois tas\n"
                  << "3. Supprimer une valeur des trois tas\n"
                  << "4. Supprimer le minimum des trois tas\n"
                  << "5. Afficher les tas dans output.txt\n"
                  << "6. Lancer le benchmark aleatoire\n"
                  << "0. Quitter\n";

        const int choice = readInteger("Votre choix : ");
        if (choice == 0) {
            return;
        }

        if (choice == 1) {
            const int value = readInteger("Valeur a inserer : ");
            vectorHeap.insert(value);
            listHeap.insert(value);
            mapHeap.insert(value);
            std::cout << "Valeur inseree dans les trois tas.\n";
        } else if (choice == 2) {
            const int value = readInteger("Valeur a rechercher : ");
            std::cout << "vector : " << (vectorHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "list   : " << (listHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "map    : " << (mapHeap.contains(value) ? "trouvee" : "absente") << '\n';
        } else if (choice == 3) {
            const int value = readInteger("Valeur a supprimer : ");
            std::cout << "vector : " << (vectorHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "list   : " << (listHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "map    : " << (mapHeap.erase(value) ? "supprimee" : "absente") << '\n';
        } else if (choice == 4) {
            if (vectorHeap.empty()) {
                std::cout << "Les tas sont vides : aucun minimum a supprimer.\n";
                continue;
            }
            std::cout << "Minimum supprime : vector=" << vectorHeap.removeMinimum()
                      << ", list=" << listHeap.removeMinimum()
                      << ", map=" << mapHeap.removeMinimum() << '\n';
        } else if (choice == 5) {
            saveInteractiveHeaps(vectorHeap, listHeap, mapHeap);
            std::cout << "Les trois tas ont ete ecrits dans output.txt.\n";
        } else if (choice == 6) {
            const int requestedCount = readInteger("Nombre d'elements a tester (0 a 200000) : ");
            if (requestedCount < 0 || requestedCount > 200000) {
                std::cout << "Le nombre doit etre compris entre 0 et 200000.\n";
                continue;
            }

            const std::size_t count = static_cast<std::size_t>(requestedCount);
            std::mt19937 generator(std::random_device{}());
            std::uniform_int_distribution<int> distribution(
                1, std::max(10, requestedCount * 10));

            std::vector<int> values;
            values.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(distribution(generator));
            }

            const std::size_t queryCount = std::min<std::size_t>(count, 1000);
            std::vector<int> queries;
            queries.reserve(queryCount);
            for (std::size_t i = 0; i < queryCount; ++i) {
                std::uniform_int_distribution<std::size_t> pick(0, count - 1);
                queries.push_back(values[pick(generator)]);
            }

            std::vector<BenchmarkResult> results;
            results.push_back(runBenchmark<VectorStore>("vector", values, queries));
            results.push_back(runBenchmark<ListStore>("list", values, queries));
            results.push_back(runBenchmark<MapStore>("map", values, queries));
            saveBenchmarkResults(count, results);

            std::cout << "\nBenchmark termine. Resultats enregistres dans output.txt\n"
                      << std::fixed << std::setprecision(3);
            for (const BenchmarkResult& result : results) {
                std::cout << result.containerName
                          << " : insertion " << result.insertionMs << " ms, recherche "
                          << result.searchMs << " ms, suppression "
                          << result.deletionMs << " ms, affichage "
                          << result.displayMs << " ms.\n";
            }
        } else {
            std::cout << "Choix invalide.\n";
        }
    }
}

int main() {
    try {
        runInteractiveMenu();
    } catch (const std::exception& error) {
        std::cerr << "\nErreur : " << error.what() << '\n';
        return 1;
    }
    return 0;
}
