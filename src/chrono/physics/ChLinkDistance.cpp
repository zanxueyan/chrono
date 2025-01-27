// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Alessandro Tasora, Radu Serban
// =============================================================================

#include "chrono/physics/ChLinkDistance.h"

namespace chrono {

// Register into the object factory, to enable run-time dynamic creation and persistence
CH_FACTORY_REGISTER(ChLinkDistance)

class ChLinkDistance_Mode_enum_mapper : public ChLinkDistance {
  public:
    CH_ENUM_MAPPER_BEGIN(Mode);
    CH_ENUM_VAL(Mode::BILATERAL);
    CH_ENUM_VAL(Mode::UNILATERAL_MAXDISTANCE);
    CH_ENUM_VAL(Mode::UNILATERAL_MINDISTANCE);
    CH_ENUM_MAPPER_END(Mode);
};

ChLinkDistance::ChLinkDistance() : pos1(VNULL), pos2(VNULL), distance(0), curr_dist(0) {
    this->SetMode(Mode::BILATERAL);
}

ChLinkDistance::ChLinkDistance(const ChLinkDistance& other) : ChLink(other) {
    this->SetMode(other.mode);
    Body1 = other.Body1;
    Body2 = other.Body2;
    system = other.system;
    Cx.SetVariables(&other.Body1->Variables(), &other.Body2->Variables());
    pos1 = other.pos1;
    pos2 = other.pos2;
    distance = other.distance;
    curr_dist = other.curr_dist;
}

int ChLinkDistance::Initialize(std::shared_ptr<ChBodyFrame> mbody1,
                               std::shared_ptr<ChBodyFrame> mbody2,
                               bool pos_are_relative,
                               ChVector<> mpos1,
                               ChVector<> mpos2,
                               bool auto_distance,
                               double mdistance,
                               Mode mode) {
    this->SetMode(mode);

    Body1 = mbody1.get();
    Body2 = mbody2.get();
    Cx.SetVariables(&Body1->Variables(), &Body2->Variables());

    if (pos_are_relative) {
        pos1 = mpos1;
        pos2 = mpos2;
    } else {
        pos1 = Body1->TransformPointParentToLocal(mpos1);
        pos2 = Body2->TransformPointParentToLocal(mpos2);
    }

    ChVector<> delta_pos = Body1->TransformPointLocalToParent(pos1) - Body2->TransformPointLocalToParent(pos2);
    curr_dist = delta_pos.Length();

    if (auto_distance) {
        distance = curr_dist;
    } else {
        distance = mdistance;
    }

    C[0] = mode_sign * (curr_dist - distance);

    return true;
}

void ChLinkDistance::SetMode(Mode mode) {
    this->mode = mode;
    mode_sign = (this->mode == Mode::UNILATERAL_MAXDISTANCE ? -1.0 : +1.0);
    Cx.SetMode(this->mode == Mode::BILATERAL ? eChConstraintMode::CONSTRAINT_LOCK
                                             : eChConstraintMode::CONSTRAINT_UNILATERAL);
}

ChCoordsys<> ChLinkDistance::GetLinkRelativeCoords() {
    ChVector<> dir_F1_F2_W =
        (Vnorm(Body1->TransformPointLocalToParent(pos1) - Body2->TransformPointLocalToParent(pos2)));
    ChVector<> dir_F1_F2_B1 = Body2->TransformDirectionParentToLocal(dir_F1_F2_W);
    ChVector<> Vx, Vy, Vz;
    XdirToDxDyDz(dir_F1_F2_B1, VECT_Y, Vx, Vy, Vz);
    ChMatrix33<> rel_matrix(Vx, Vy, Vz);

    Quaternion Ql2 = rel_matrix.Get_A_quaternion();
    return ChCoordsys<>(pos2, Ql2);
}

void ChLinkDistance::Update(double mytime, bool update_assets) {
    // Inherit time changes of parent class (ChLink), basically doing nothing :)
    ChLink::Update(mytime, update_assets);

    // compute jacobians
    ChVector<> delta_pos = Body1->TransformPointLocalToParent(pos1) - Body2->TransformPointLocalToParent(pos2);
    curr_dist = delta_pos.Length();
    ChVector<> dir_F1_F2_W = Vnorm(delta_pos);
    ChVector<> dir_F1_F2_B2 = Body2->TransformDirectionParentToLocal(dir_F1_F2_W);
    ChVector<> dir_F1_F2_B1 = Body1->TransformDirectionParentToLocal(dir_F1_F2_W);

    ChVector<> Cq_B1_pos = dir_F1_F2_W;
    ChVector<> Cq_B2_pos = -dir_F1_F2_W;

    ChVector<> Cq_B1_rot = -Vcross(dir_F1_F2_B1, pos1);
    ChVector<> Cq_B2_rot = Vcross(dir_F1_F2_B2, pos2);

    Cx.Get_Cq_a()(0) = mode_sign * Cq_B1_pos.x();
    Cx.Get_Cq_a()(1) = mode_sign * Cq_B1_pos.y();
    Cx.Get_Cq_a()(2) = mode_sign * Cq_B1_pos.z();
    Cx.Get_Cq_a()(3) = mode_sign * Cq_B1_rot.x();
    Cx.Get_Cq_a()(4) = mode_sign * Cq_B1_rot.y();
    Cx.Get_Cq_a()(5) = mode_sign * Cq_B1_rot.z();

    Cx.Get_Cq_b()(0) = mode_sign * Cq_B2_pos.x();
    Cx.Get_Cq_b()(1) = mode_sign * Cq_B2_pos.y();
    Cx.Get_Cq_b()(2) = mode_sign * Cq_B2_pos.z();
    Cx.Get_Cq_b()(3) = mode_sign * Cq_B2_rot.x();
    Cx.Get_Cq_b()(4) = mode_sign * Cq_B2_rot.y();
    Cx.Get_Cq_b()(5) = mode_sign * Cq_B2_rot.z();

    C[0] = mode_sign * (curr_dist - distance);

    //// TODO  C_dt? C_dtdt? (may be never used..)
}

//// STATE BOOKKEEPING FUNCTIONS

void ChLinkDistance::IntStateGatherReactions(const unsigned int off_L, ChVectorDynamic<>& L) {
    L(off_L) = -react_force.x();
}

void ChLinkDistance::IntStateScatterReactions(const unsigned int off_L, const ChVectorDynamic<>& L) {
    react_force.x() = -L(off_L);
    react_force.y() = 0;
    react_force.z() = 0;

    react_torque = VNULL;
}

void ChLinkDistance::IntLoadResidual_CqL(const unsigned int off_L,    ///< offset in L multipliers
                                         ChVectorDynamic<>& R,        ///< result: the R residual, R += c*Cq'*L
                                         const ChVectorDynamic<>& L,  ///< the L vector
                                         const double c               ///< a scaling factor
) {
    if (!IsActive())
        return;

    Cx.MultiplyTandAdd(R, L(off_L) * c);
}

void ChLinkDistance::IntLoadConstraint_C(const unsigned int off_L,  ///< offset in Qc residual
                                         ChVectorDynamic<>& Qc,     ///< result: the Qc residual, Qc += c*C
                                         const double c,            ///< a scaling factor
                                         bool do_clamp,             ///< apply clamping to c*C?
                                         double recovery_clamp      ///< value for min/max clamping of c*C
) {
    if (!IsActive())
        return;

    if (do_clamp)
        if (mode == Mode::BILATERAL)
            Qc(off_L) += ChMin(ChMax(c * C[0], -recovery_clamp), recovery_clamp);
        else
            Qc(off_L) += ChMax(c * C[0], -recovery_clamp);
    else
        Qc(off_L) += c * C[0];
}

void ChLinkDistance::IntToDescriptor(const unsigned int off_v,
                                     const ChStateDelta& v,
                                     const ChVectorDynamic<>& R,
                                     const unsigned int off_L,
                                     const ChVectorDynamic<>& L,
                                     const ChVectorDynamic<>& Qc) {
    if (!IsActive())
        return;

    Cx.Set_l_i(L(off_L));

    Cx.Set_b_i(Qc(off_L));
}

void ChLinkDistance::IntFromDescriptor(const unsigned int off_v,
                                       ChStateDelta& v,
                                       const unsigned int off_L,
                                       ChVectorDynamic<>& L) {
    if (!IsActive())
        return;

    L(off_L) = Cx.Get_l_i();
}

// SOLVER INTERFACES

void ChLinkDistance::InjectConstraints(ChSystemDescriptor& mdescriptor) {
    if (!IsActive())
        return;

    mdescriptor.InsertConstraint(&Cx);
}

void ChLinkDistance::ConstraintsBiReset() {
    Cx.Set_b_i(0.);
}

void ChLinkDistance::ConstraintsBiLoad_C(double factor, double recovery_clamp, bool do_clamp) {
    if (!IsActive())
        return;

    if (do_clamp)
        Cx.Set_b_i(Cx.Get_b_i() + ChMin(ChMax(factor * C[0], -recovery_clamp), recovery_clamp));
    else
        Cx.Set_b_i(Cx.Get_b_i() + factor * C[0]);
}

void ChLinkDistance::ConstraintsLoadJacobians() {
    // already loaded when doing Update (which used the matrices of the scalar constraint objects)
}

void ChLinkDistance::ConstraintsFetch_react(double factor) {
    // From constraints to react vector:
    react_force.x() = -Cx.Get_l_i() * factor;
    react_force.y() = 0;
    react_force.z() = 0;

    react_torque = VNULL;
}

void ChLinkDistance::ArchiveOut(ChArchiveOut& marchive) {
    // version number
    marchive.VersionWrite<ChLinkDistance>();

    // serialize parent class
    ChLink::ArchiveOut(marchive);

    // serialize all member data:
    marchive << CHNVP(distance);
    marchive << CHNVP(pos1);
    marchive << CHNVP(pos2);

    ChLinkDistance_Mode_enum_mapper::Mode_mapper typemapper;
    marchive << CHNVP(typemapper(mode), "ChLinkDistance__Mode");
}

/// Method to allow de serialization of transient data from archives.
void ChLinkDistance::ArchiveIn(ChArchiveIn& marchive) {
    // version number
    /*int version =*/marchive.VersionRead<ChLinkDistance>();

    // deserialize parent class
    ChLink::ArchiveIn(marchive);

    // deserialize all member data:
    marchive >> CHNVP(distance);
    marchive >> CHNVP(pos1);
    marchive >> CHNVP(pos2);

    Cx.SetVariables(&Body1->Variables(), &Body2->Variables());

    ChLinkDistance_Mode_enum_mapper::Mode_mapper typemapper;
    Mode mode_temp;
    marchive >> CHNVP(typemapper(mode_temp), "ChLinkDistance__Mode");
    SetMode(mode_temp);
}

}  // namespace chrono
