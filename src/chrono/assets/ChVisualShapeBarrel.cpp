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
// Authors: Alessandro Tasora
// =============================================================================

#include "chrono/assets/ChVisualShapeBarrel.h"

namespace chrono {

// Register into the object factory, to enable run-time
// dynamic creation and persistence
CH_FACTORY_REGISTER(ChVisualShapeBarrel)

ChVisualShapeBarrel::ChVisualShapeBarrel() {
    SetMutable(false);
}

ChVisualShapeBarrel::ChVisualShapeBarrel(double Y_low, double Y_high, double axis_vert, double axis_hor, double R_offset)
    : Hlow(Y_low), Hsup(Y_high), Rvert(axis_vert / 2), Rhor(axis_hor / 2), Roffset(R_offset) {
    SetMutable(false);
}

void ChVisualShapeBarrel::ArchiveOut(ChArchiveOut& marchive) {
    // version number
    marchive.VersionWrite<ChVisualShapeBarrel>();
    // serialize parent class
    ChVisualShape::ArchiveOut(marchive);
    // serialize all member data:
    marchive << CHNVP(Hlow);
    marchive << CHNVP(Hsup);
    marchive << CHNVP(Rvert);
    marchive << CHNVP(Rhor);
    marchive << CHNVP(Roffset);
}

/// Method to allow de serialization of transient data from archives.
void ChVisualShapeBarrel::ArchiveIn(ChArchiveIn& marchive) {
    // version number
    /*int version =*/marchive.VersionRead<ChVisualShapeBarrel>();
    // deserialize parent class
    ChVisualShape::ArchiveIn(marchive);
    // stream in all member data:
    marchive >> CHNVP(Hlow);
    marchive >> CHNVP(Hsup);
    marchive >> CHNVP(Rvert);
    marchive >> CHNVP(Rhor);
    marchive >> CHNVP(Roffset);
}

}  // end namespace chrono
