#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>

#include <shape.hpp>
#include <svg_gen.h>

static const int SVG_CELL_SIDE = 10; 
static const int GENERATION_SIZE = 10000;
static const int NUM_ITER = 1000;

static const int NUM_ELITE = 1;
static const int NUM_MUTATED = (int)(GENERATION_SIZE*0.9);
static const int SEED = 12345;

static const int NUM_RETRIES = 1000;
static const int MIN_FLIPS = 2;
static const int MAX_FLIPS = 4;

static const int ITER_DUMP_AFTER = 1;

int main(int argc, char* argv[]) {

    std::string shape_file = "data/pentominoes.txt";

    if (argc > 1) shape_file = argv[1];

    std::ifstream ifs(shape_file);
    std::string line;

    std::vector<shape> shapes = shape::parse(ifs);

    shape::variation_array variations;
    for (const auto& sh: shapes) variations.push_back(sh.get_variations());

    //  estimate the radius
    double len = 0.0;
    for (const shape& sh : shapes) len += sh.estimate_len();
    double R = len/(2.0*PI);

    const int nshapes = (int)shapes.size();
    
    srand(SEED);

    std::vector<std::vector<shape_pos>> gen[2];
    gen[0].resize(GENERATION_SIZE);
    gen[1].resize(GENERATION_SIZE);

    struct lscore {
        std::vector<shape_pos>* pos;
        double score;
        bool operator <(const lscore& rhs) const {return score > rhs.score; }
        bool operator ==(const lscore& rhs) const {return *pos == *rhs.pos; }
    };
    std::vector<lscore> scores(GENERATION_SIZE);

    using namespace std::chrono;
    high_resolution_clock::time_point start_time = high_resolution_clock::now();

    auto* cur_gen  = &gen[0];
    auto* prev_gen = &gen[1];

    //  seed the first generation
    for (int k = 0; k < GENERATION_SIZE; k++) {
        auto& pos = (*cur_gen)[k];
        pos.resize(nshapes, {0, 0, 0, 0});
        for (int i = 0; i < nshapes; i++) pos[i].shape_idx = i;

        //std::random_shuffle(pos.begin(), pos.end());
        shape::arrange_circle(R, variations, pos);
        scores[k].score = shape::score(variations, pos);
        scores[k].pos = &pos;
    }
    std::sort(scores.begin(), scores.end());

    for (int it = 0; it < NUM_ITER; it++) {
        std::swap(cur_gen, prev_gen);

        int ii = 0;
        //  transfer the "elite" ones (making sure there is no duplicates)
        for (int i = 0; i < GENERATION_SIZE; i++) {
            const auto& pos = *(scores[i].pos);
            bool dupe = false;
            for (int j = 0; j < ii; j++) {
                if ((*cur_gen)[j] == pos) {
                    dupe = true;
                    break;
                } 
            }
            if (!dupe) (*cur_gen)[ii++] = pos;
            if (ii == NUM_ELITE) break;
        }

        //  apply the mutations
        for (int i = 0; i < NUM_MUTATED; i++) {
            // pick the source gene
            int pick_size = GENERATION_SIZE;
            int idx = (int)sqrtf((float)(rand()%(pick_size*pick_size)));
            const auto& src = *(scores[idx].pos);
            auto& dst = (*cur_gen)[ii++];

            std::vector<shape_pos> max_target;
            double max_score = -std::numeric_limits<double>::max();

            for (int ii = 0; ii < NUM_RETRIES; ii++) {
                std::vector<shape_pos> target = src;

                int num_flips = rand()%(MAX_FLIPS - MIN_FLIPS + 1) + MIN_FLIPS;
                for (int iii = 0; iii < num_flips; iii++) {
                    int mutation = rand()%3;
                    int pidx1 = rand()%nshapes;
                    int pidx2 = rand()%nshapes;

                    if (mutation == 0) {
                        target[pidx1].var_idx = (uint16_t)(rand()%variations[target[pidx1].shape_idx].size());
                        target[pidx2].var_idx = (uint16_t)(rand()%variations[target[pidx2].shape_idx].size());
                    } else if (mutation == 1) {
                        const vec2i& offs = COFFS[rand()%8];
                        for (int k = pidx1;  k <= pidx2; k++) {
                            target[k].x += offs.x;
                            target[k].y += offs.y;
                        }
                    } else if (mutation == 2) {
                        std::swap(target[pidx1].shape_idx, target[pidx2].shape_idx);
                        std::swap(target[pidx1].var_idx, target[pidx2].var_idx);
                    }
                }

                double score = shape::score(variations, target);
                if (score > max_score) {
                    max_score = score;
                    max_target = target;
                }
            }

            dst = max_target;
        }

        //  pad the rest with the fresh ones
        for (; ii < GENERATION_SIZE; ii++) {
            auto& pos = (*cur_gen)[ii];
            pos.resize(nshapes, {0, 0, 0, 0});
            for (int i = 0; i < nshapes; i++) pos[i].shape_idx = i;
            std::random_shuffle(pos.begin(), pos.end());
            shape::arrange_circle(R, variations, pos);
        }

        //  score the current generation
        for (int k = 0; k < GENERATION_SIZE; k++) {
            auto& pos = (*cur_gen)[k];
            shape::center(variations, pos);
            scores[k].score = shape::score(variations, pos);
            scores[k].pos = &pos;
        }
        std::sort(scores.begin(), scores.end());

        high_resolution_clock::time_point cur_time = high_resolution_clock::now();
        auto int_ms = duration_cast<std::chrono::milliseconds>(cur_time - start_time);
        std::cout << "Iteration: " << it << ", max score: " << scores[0].score << 
            ", time: " << int_ms.count() << "ms" << std::endl;
        start_time = cur_time;

        if ((it%ITER_DUMP_AFTER == 0) || it == NUM_ITER - 1) {
            std::ofstream ofs("out/test.html");
            ofs << "<div>\n";    

            int ndisp = std::min(100, GENERATION_SIZE);
            int k = 0, cur_pos = 0;
            while (cur_pos < ndisp && k < GENERATION_SIZE) {
                const auto& pos = *(scores[k].pos);

                //  check if a duplicate
                bool dupe = false;
                for (int kk = 0; kk < k; kk++) {
                    if (pos == *(scores[kk].pos)) dupe = true;
                }

                k++;
                if (dupe) continue;
                cur_pos++;

                shape core;
                vec2i core_pos;
                bool has_core = shape::extract_core(variations, pos, core, core_pos);
                if (has_core) {
                    create_svg(ofs, variations, pos, &core, &core_pos, SVG_CELL_SIDE);
                } else {
                    create_svg(ofs, variations, pos, nullptr, nullptr, SVG_CELL_SIDE);
                }
            }

            ofs << "</div>\n";
            ofs.close();   
        }
    }
    
    return 0;
}

