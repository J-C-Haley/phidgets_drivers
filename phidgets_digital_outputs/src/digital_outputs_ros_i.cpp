/*
 * Copyright (c) 2019, Open Source Robotics Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <memory>

#include <ros/ros.h>
#include <std_msgs/Bool.h>

#include "phidgets_api/digital_outputs.h"
#include "phidgets_digital_outputs/digital_outputs_ros_i.h"
#include "phidgets_msgs/SetDigitalOutput.h"

namespace phidgets {

DigitalOutputsRosI::DigitalOutputsRosI(ros::NodeHandle nh,
                                       ros::NodeHandle nh_private)
    : nh_(nh), nh_private_(nh_private)
{
    ROS_INFO("Starting Phidgets Digital Outputs");

    int serial_num;
    if (!nh_private_.getParam("serial", serial_num))
    {
        serial_num = -1;  // default open any device
    }
    int hub_port;
    if (!nh_private.getParam("hub_port", hub_port))
    {
        hub_port = 0;  // only used if the device is on a VINT hub_port
    }
    bool is_hub_port_device;
    if (!nh_private.getParam("is_hub_port_device", is_hub_port_device))
    {
        // only used if the device is on a VINT hub_port
        is_hub_port_device = false;
    }
    if (nh_private.getParam("server_name", server_name_) 
        && nh_private.getParam("server_ip", server_ip_)){

        PhidgetNet_addServer(server_name_.c_str(), server_ip_.c_str(), 5661, "", 0);

        ROS_INFO(
            "Using phidget server %s at IP %s",
            server_name_.c_str(), server_ip_.c_str());
    }

    ROS_INFO("Connecting to Phidgets DigitalOutputs serial %d, hub port %d ...",
             serial_num, hub_port);

    try
    {
        dos_ = std::make_unique<DigitalOutputs>(serial_num, hub_port,
                                                is_hub_port_device);

    } catch (const Phidget22Error& err)
    {
        ROS_ERROR("DigitalOutputs: %s", err.what());
        throw;
    }

    int n_out = dos_->getOutputCount();
    ROS_INFO("Connected %d outputs", n_out);
    out_subs_.resize(n_out);
    for (int i = 0; i < n_out; i++)
    {
        char topicname[] = "digital_output00";
        snprintf(topicname, sizeof(topicname), "digital_output%02d", i);
        out_subs_[i] =
            std::make_unique<DigitalOutputSetter>(dos_.get(), i, nh, topicname);
    }
    out_srv_ = nh_.advertiseService("set_digital_output",
                                    &DigitalOutputsRosI::setSrvCallback, this);
}

bool DigitalOutputsRosI::setSrvCallback(
    phidgets_msgs::SetDigitalOutput::Request& req,
    phidgets_msgs::SetDigitalOutput::Response& res)
{
    dos_->setOutputState(req.index, req.state);
    res.success = true;
    return true;
}

DigitalOutputSetter::DigitalOutputSetter(DigitalOutputs* dos, int index,
                                         ros::NodeHandle nh,
                                         const std::string& topicname)
    : dos_(dos), index_(index)
{
    subscription_ =
        nh.subscribe(topicname, 1, &DigitalOutputSetter::setMsgCallback, this);
}

void DigitalOutputSetter::setMsgCallback(const std_msgs::Bool::ConstPtr& msg)
{
    dos_->setOutputState(index_, msg->data);
}

}  // namespace phidgets
