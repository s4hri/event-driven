/*
 * Copyright (C) 2014 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Arren Glover (@itt.it)
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

/**
 * \defgroup
 * @ingroup emorph_lib
 *
 * Data transport method for the eMorph project
 *
 * Author: Arren Glover 2014
 * Copyright (C) 2010 RobotCub Consortium
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
*/


#ifndef __vBottle__
#define __vBottle__


#include <yarp/os/Bottle.h>
#include <iCub/emorph/vCodec.h>
#include <iCub/emorph/vQueue.h>

namespace emorph {

class vBottle : public yarp::os::Bottle {

public:

    //constructors shouldn't change from Bottle
    vBottle() : yarp::os::Bottle() {}

    //you can only modify contents by adding events and append other vBottles
    void addEvent(emorph::vEvent &e);
    void append(vBottle &eb);

    //all get functions call this to do the meat of the getting function
    template<class T> vQueue get() {

        vQueue q;
        addtoendof<T>(q);
        return q;

    }

    template<class T> void addtoendof(vQueue &q) {

        //the bottle is stored as TAG (EVENTS) TAG (EVENTS)
        for(int i = 0; i < Bottle::size(); i+=2) {

            //so for each TAG we create an event of that type
            vEvent * e = emorph::createEvent(Bottle::get(i).asString());
            if(!e) {
                std::cerr << "Warning: could not get bottle type during vBottle::"
                             "get<>(). Check vBottle integrity." << std::endl;
                continue;
            }

            //and if e is of type T we can continue to get the events
            if(!dynamic_cast<T*>(e)) continue;

            //we get the (EVENTS)
            Bottle * b = Bottle::get(i+1).asList();
            if(!b) {
                std::cerr << "Warning: could not get event data as a list after "
                             "getting correct tag (e.g. AE) in vBottle::getAll(). "
                             "Check vBottle integrity" << std::endl;
                delete(e);
                break;
            }

            //and decode each one also creating the memory with clone
            int pos = 0;
            while(pos < b->size()) {
                if(e->decode(*b, pos)) {
                    q.push_back(e->clone());
                }
            }

            //finally we don't need the last event
            delete(e);

        }

    }

    template<class T> vQueue getSorted()
    {
        vQueue q = get<T>();
        q.sort();
        return q;
    }

    vQueue getAll() {
        vQueue q = this->get<vEvent>(); //all events are of type vEvent
        return q;
    }

    vQueue getAllSorted() {
        vQueue q = getAll();
        q.sort();
        return q;
    }

    //you can also access the following functions
    //not sure if some need to be blocked

//    Bottle::clear()
//    Bottle::check()
//    Bottle::fromBinary()
//    Bottle::fromString()
//    Bottle::getContext()
//    Bottle::getMonitor()
//    Bottle::getNullBottle()
//    Bottle::getReadType()
//    Bottle::getSpecialization();
//    Bottle::getType()
//    Bottle::getWriteType()
//    Bottle::hasChanged()
//    Bottle::isNull()
//    Bottle::onCommencement()
//    Bottle::onCompletion()
//    Bottle::operator !=()
//    Bottle::operator =()
//    Bottle::operator ==()
//    Bottle::setMonitor()
//    Bottle::setNested()
//    Bottle::size()
//    Bottle::specialize()
//    Bottle::Searchable
//    Bottle::toBinary()
//    Bottle::toString()


private:

    //you cannot use any of the following functions
    void add();
    void addDict();
    void addDouble();
    void addInt();
    void addList();
    void addString();
    void addVocab();
    void fromString(const yarp::os::ConstString& text);
    void append(const yarp::os::Bottle &alt);
    void copy();
    void copyPortable();
    //yarp::os::Value& find(const yarp::os::ConstString &key);
    //yarp::os::Value& find(const ConstString &key) : Bottle::find(const yarp::os::ConstString &key) {};
    void findGroup();
    yarp::os::Bottle tail() const;


    int getInt();
    yarp::os::ConstString getString();
    double getDouble();
    Bottle* getList();
    yarp::os::Value& get();

    yarp::os::Value pop();

};

class vBottleMimic : public yarp::os::Portable {

private:

    std::vector<u_int32_t> header1;
    std::vector<char> header2;
    std::vector<u_int32_t> header3;

    const char * datablock;
    unsigned int datalength;
    const static unsigned int MINELSZ = sizeof(u_int32_t) * 2;

public:

    vBottleMimic() {
        header1.push_back(BOTTLE_TAG_LIST); //bottle code
        header1.push_back(2); //elements in bottle "AE" then bottle data
        header1.push_back(BOTTLE_TAG_STRING); //code for string
        header1.push_back(2); // length of string
        header2.push_back('A');
        header2.push_back('E');
        header3.push_back(BOTTLE_TAG_LIST|BOTTLE_TAG_INT); // bottle code + specialisation with ints
        header3.push_back(0); // <- set the number of ints here (2 * #v's)
    }

    void setdata(const char * datablock, unsigned int datalength) {
        header3[1] = 2 * (datalength / MINELSZ); //forced to be x8
        this->datablock = datablock;
        this->datalength = MINELSZ * header3[1] / 2; //forced to be x8

    }

    virtual bool read(yarp::os::ConnectionReader& connection) {
                return false;
    }

    virtual bool write(yarp::os::ConnectionWriter& connection) {

        connection.appendExternalBlock((const char *)header1.data(),
                                       header1.size() * sizeof(u_int32_t));
        connection.appendExternalBlock((const char *)header2.data(),
                                       header2.size() * sizeof(char));
        connection.appendExternalBlock((const char *)header3.data(),
                                       header3.size() * sizeof(u_int32_t));
        connection.appendExternalBlock(datablock, datalength);

        return !connection.isError();
    }

};

} //end namespace emorph

#endif /*__vBottle__*/

//----- end-of-file --- ( next line intentionally left blank ) ------------------
