#ifndef MIIC_PROBA_ORIENTATION_H_
#define MIIC_PROBA_ORIENTATION_H_

namespace miic {
namespace reconstruction {

double *getOrientTplLVDegPropag(int, int *, double *, int, int, bool, bool);
int OrientTpl_LV_Deg_Propag(int NbTpl, int *Tpl, double *I3,
    double *ProbArrowhead, int LV, int deg, bool propagation,
    bool half_v_structure);

}  // namespace reconstruction
}  // namespace miic

#endif  // MIIC_PROBA_ORIENTATION_H_
