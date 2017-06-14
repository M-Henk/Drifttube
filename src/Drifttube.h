/*
 * Drifttube.h
 *
 *  Created on: Apr 6, 2017
 *      Author: bieschke
 */

#ifndef DRIFTTUBE_H_
#define DRIFTTUBE_H_

#include <array>
#include <memory>
#include "DataSet.h"


/**
 * Basic implementation of a drifttube. This contains the coordinates of the drift tube as well as
 * a DataSet object containing its raw data.
 *
 * @brief Representation of a tube detector.
 *
 * @author Stefan Bieschke
 * @date April 6, 2017
 * @version Alpha 2.0
 */
class Drifttube
{
public:
	Drifttube(const int posX, const int posY, unique_ptr<DataSet> data);
	~Drifttube();

	const unsigned int getRadius() const;
	const int getPositionX() const;
	const int getPositionY() const;
	const std::array<int,2>& getPosition() const;

	const DataSet& getDataSet() const;

private:
	const unsigned int m_radius = 18150; //micron
	std::array<int,2> m_position;
	std::unique_ptr<DataSet> m_data;
};

#endif /* DRIFTTUBE_H_ */