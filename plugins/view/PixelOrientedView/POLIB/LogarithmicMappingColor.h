/**
 *
 * This file is part of Tulip (http://tulip.labri.fr)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */

#ifndef LOGARITHMICMAPPINGCOLOR_H
#define LOGARITHMICMAPPINGCOLOR_H

#include "potypes.h"
#include "LinearMappingColor.h"

namespace pocore {
class LogarithmicMappingColor : public LinearMappingColor {
public:
  LogarithmicMappingColor(const double &min, const double &max);
  RGBA getColor(const double &value, const unsigned int) const override;
};
} // namespace pocore
#endif // LOGARITHMICMAPPINGCOLOR_H
