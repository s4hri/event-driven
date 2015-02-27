/*
 * Copyright (C) 2014 Istituto Italiano di Tecnologia
 * Author: Arren Glover
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

#include <iostream>
#include <string>
#include <eventAnalysis.h>
//#include <iCub/emorph/eventCodec.h>

/******************************************************************************/
//EVENT STATISTICS DUMPER
/******************************************************************************/

eventStatisticsDumper::eventStatisticsDumper()
{


    this->setStrict();
    dir = "";
    prevstamp = 0;
    bottle_number = 0;
}

bool eventStatisticsDumper::open(std::string moduleName)
{

    //allow input of vBottles
    this->useCallback();
    std::string inPortName = "/" + moduleName + "/vBottle:i";
    BufferedPort<emorph::vBottle>::open( inPortName.c_str() );



    //now initialise and open all our writers
    std::cout << "Opening writers: " << std::endl;
    std::string fname;

    //wrap_stats
    fname = dir + "wrapStats.txt";
    wrap_writer.open(fname.c_str());
    if(!wrap_writer.is_open()) {
        std::cerr << "File did not open at: " << fname << std::endl;
    } else {
        std::cout << fname << " successfully opened" << std::endl;
    }
    wrap_writer << "Event Statistics Output File" << std::endl;
    wrap_writer << "Bottle# Event#/#inBottle PrevStamp CurStamp ..." << std::endl;

    //vBottle size stats
    fname = dir + "bottleSize.txt";
    count_writer.open(fname.c_str());
    if(!count_writer.is_open()) {
        std::cerr << "File did not open at: " << fname << std::endl;
    } else {
        std::cout << fname << " successfully opened" << std::endl;
    }
    count_writer << "Bottle Size | most recent stamp" << std::endl;

    return true;
}

void eventStatisticsDumper::close()
{
    std::cout << "now closing ports..." << std::endl;
    wrap_writer.close();
    BufferedPort<emorph::vBottle>::close();
    std::cout << "finished closing the read port..." << std::endl;
}

void eventStatisticsDumper::interrupt()
{
    fprintf(stdout,"cleaning up...\n");
    fprintf(stdout,"attempting to interrupt ports\n");
    BufferedPort<emorph::vBottle>::interrupt();
    fprintf(stdout,"finished interrupt ports\n");
}

void eventStatisticsDumper::onRead(emorph::vBottle &bot)
{

    bottle_number++;
    if(bottle_number % 1000 == 0) {
        //every 1000 bottles put a message so we know the module is
        //actaully running in case of good operation
        wrap_writer << bottle_number << ": ... " << std::endl;
    }
    //std::cout << ". ";

    //create event queue
    emorph::vQueue q;
    emorph::vQueue::iterator qi, qi2;
    bot.getAll(q);

    int i = 0, j = 0;

    for(qi = q.begin(); qi != q.end(); qi++)
    {
        i++;
        if ((*qi)->getStamp() < prevstamp) {
            wrap_writer << bottle_number << ": " << i << "/" << q.size() << " : ";
            wrap_writer << prevstamp << " " << (*qi)->getType() <<
                       (*qi)->getStamp() << " ";
            for(j = 0, qi2 = qi; j < 4 && qi2 != q.end(); j++, qi2++)
                wrap_writer << (*qi2)->getType() << (*qi2)->getStamp() << " ";
            wrap_writer << std::endl;

        }
        prevstamp = (*qi)->getStamp();
    }

    count_writer << q.size();
    if(q.size()) count_writer << " | " << q.back()->getStamp();
    count_writer << std::endl;
}


/******************************************************************************/
//EVENT STATISTICS MODULE
/******************************************************************************/

eventStatisticsModule::eventStatisticsModule()
{

}

bool eventStatisticsModule::configure(yarp::os::ResourceFinder &rf)
{

    std::string name = rf.check("name",
                                yarp::os::Value("ESD")
                                ).asString();

    std::string dir = rf.check("dir",
                               yarp::os::Value("")).asString();
    setName(name.c_str());

    esd.setDirectory(dir);
    esd.open(name);

    return true;
}

bool eventStatisticsModule::close()
{

    std::cout << "Closing the Event Statistics Module" << std::endl;
    esd.close();
}

bool eventStatisticsModule::updateModule()
{
//    std::cout << name << " " << esd.getBatchedPercentage() << "% batched "
//              << esd.getBatchedCount() << "# more "
//              << esd.getTSCount() << "# TS total "
//              << std::endl;

//    esd.wrap_writer << esd.getBatchedPercentage() << " "
//                << esd.getBatchedCount() << " "
//                << esd.getTSCount() << " "
//                << std::endl;

    return true;
}



