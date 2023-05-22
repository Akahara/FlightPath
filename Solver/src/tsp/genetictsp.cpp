#include "genetictsp.h"

#include <algorithm>
#include <random>
#include <numeric>
#include <iostream>
#include <assert.h>

#include "../geometry.h"

struct GeneticParameters {
    size_t generationSize;
    size_t keptIndividualCount;
    size_t generationCount;
};

constexpr GeneticParameters defaultGeneticParams()
{
    GeneticParameters params{};
    params.generationCount = 1000;
    params.generationSize = 1000;
    params.keptIndividualCount = 50;
    return params;
}

template<typename Individual>
class Genetics {
protected:
    std::mt19937 random{static_cast<std::bernoulli_distribution::result_type>(rand())}; // accessible to sub-classes
public:
    typedef float score_t;

    virtual void genRandomIndividuals(std::vector<Individual> &individuals) = 0;
    virtual score_t scoreIndividual(const Individual &ind) = 0;
    virtual Individual mutateIndividual(const Individual &parent) = 0;

    Individual runEvolution(const GeneticParameters &params=defaultGeneticParams())
    {
        assert(params.keptIndividualCount < params.generationSize);
        assert(params.keptIndividualCount > 0);

        std::vector<Individual> individuals(params.generationSize);
        std::vector<Individual> newGeneration(params.generationSize);
        std::vector<std::tuple<score_t, size_t>> scores(params.generationSize);

        genRandomIndividuals(individuals);
        
        for (size_t i = 0; i < params.generationCount; i++) {
            // score individuals
            for (size_t j = 0; j < individuals.size(); j++)
                scores[j] = { scoreIndividual(individuals[j]), j };
            // sort them by score (descending)
            std::sort(scores.begin(), scores.end(), [](auto &s1, auto &s2) { return std::get<0>(s1) > std::get<0>(s2); });
            //for (size_t i = 0; i < scores.size(); i++)
            //    std::cout << std::get<0>(scores[i]) << " ";
            //std::cout << "\n";
            // keep the M best and generate N-M new individuals by mutations
            for (size_t j = 0; j < params.keptIndividualCount; j++)
                newGeneration[j] = std::move(individuals[std::get<1>(scores[j])]);
            for (size_t j = params.keptIndividualCount; j < params.generationSize; j++)
                newGeneration[j] = mutateIndividual(newGeneration[random() % params.keptIndividualCount]);
            // swap the two arrays before running the next generation
            individuals.swap(newGeneration);
            //std::cout << "max=" << std::get<0>(scores[0]) << std::endl;
        }

        // technically the last mutation step is not required
        // but we still need to find the best individual, as-is
        // the code is not too bad but could be optimized if
        // readability is not a requirement

        // also scores for kept individuals should not be recomputed
        // this might be an issue if the evaluation function is
        // costly

        return individuals[0];
    }
};

class TSPGenetics : public Genetics<ProblemPath> {
private:
    const std::vector<ProblemStation> *m_availableStations;
public:
    TSPGenetics(const ProblemMap &map)
        : m_availableStations(&map)
    {
    }

    void genRandomIndividuals(std::vector<ProblemPath> &individuals) override
    {
        std::generate(individuals.begin(), individuals.end(), [this]() {
            std::vector<size_t> indices( m_availableStations->size() );
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), random);
            ProblemPath path;
            path.reserve(m_availableStations->size());
            for (size_t i = 0; i < indices.size(); i++)
                path.push_back(m_availableStations->at(indices[i]));
            return path;
        });
    }

    score_t scoreIndividual(const ProblemPath &ind) override
    {
        return -getLength(ind)-geometry::distance(ind[0].getLocation(), ind[ind.size() - 1].getLocation());
    }

    ProblemPath mutateIndividual(const ProblemPath &parent) override
    {
        int swapCount = 1 + random() % 3;

        ProblemPath child{ parent };

        for (size_t i = 0; i < swapCount; i++) {
            size_t s1 = random() % m_availableStations->size();
            size_t s2;
            do { s2 = random() % m_availableStations->size(); } while (s1 == s2);

            std::swap(child[s1], child[s2]);
        }

        return child;
    }
};

ProblemPath GeneticTSPSolver::solveForPath(const ProblemMap &map)
{
    TSPGenetics genetics{ map };
    return genetics.runEvolution();
}
