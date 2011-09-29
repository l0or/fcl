/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */

#include "fcl/conservative_advancement.h"
#include "test_core_utility.h"
#include "fcl/collision_node.h"
#include "fcl/simple_setup.h"


using namespace fcl;


bool CA_ccd_Test(const Transform& tf1, const Transform& tf2,
                 const std::vector<Vec3f>& vertices1, const std::vector<Triangle>& triangles1,
                 const std::vector<Vec3f>& vertices2, const std::vector<Triangle>& triangles2,
                 SplitMethodType split_method,
                 bool use_COM,
                 BVH_REAL& toc);

bool interp_ccd_Test(const Transform& tf1, const Transform& tf2,
                     const std::vector<Vec3f>& vertices1, const std::vector<Triangle>& triangles1,
                     const std::vector<Vec3f>& vertices2, const std::vector<Triangle>& triangles2,
                     SplitMethodType split_method,
                     unsigned int nsamples,
                     bool use_COM,
                     BVH_REAL& toc);

unsigned int n_dcd_samples = 10;

int main()
{
  std::vector<Vec3f> p1, p2;
  std::vector<Triangle> t1, t2;
  loadOBJFile("test/env.obj", p1, t1);
  loadOBJFile("test/rob.obj", p2, t2);

  std::vector<Transform> transforms; // t0
  std::vector<Transform> transforms2; // t1
  BVH_REAL extents[] = {-3000, -3000, 0, 3000, 3000, 3000};
  BVH_REAL delta_trans[] = {10, 10, 10};
  int n = 100;

  generateRandomTransform(extents, transforms, transforms2, delta_trans, 0.005 * 2 * 3.1415, n);

  for(unsigned int i = 0; i < transforms.size(); ++i)
  {
    std::cout << i << std::endl;
    BVH_REAL toc;
    bool res = CA_ccd_Test(transforms[i], transforms2[i], p1, t1, p2, t2, SPLIT_METHOD_MEDIAN, false, toc);

    BVH_REAL toc2;
    bool res2 = CA_ccd_Test(transforms[i], transforms2[i], p1, t1, p2, t2, SPLIT_METHOD_MEDIAN, true, toc2);

    BVH_REAL toc3;
    bool res3 = interp_ccd_Test(transforms[i], transforms2[i], p1, t1, p2, t2, SPLIT_METHOD_MEDIAN, n_dcd_samples, false, toc3);

    BVH_REAL toc4;
    bool res4 = interp_ccd_Test(transforms[i], transforms2[i], p1, t1, p2, t2, SPLIT_METHOD_MEDIAN, n_dcd_samples, true, toc4);


    if(res) std::cout << "yes "; else std::cout << "no ";
    if(res2) std::cout << "yes "; else std::cout << "no ";
    if(res3) std::cout << "yes "; else std::cout << "no ";
    if(res4) std::cout << "yes "; else std::cout << "no ";
    std::cout << std::endl;

    std::cout << toc << " " << toc2 << " " << toc3 << " " << toc4 << std::endl;
    std::cout << std::endl;
  }

  return 1;

}


bool CA_ccd_Test(const Transform& tf1, const Transform& tf2,
                 const std::vector<Vec3f>& vertices1, const std::vector<Triangle>& triangles1,
                 const std::vector<Vec3f>& vertices2, const std::vector<Triangle>& triangles2,
                 SplitMethodType split_method,
                 bool use_COM,
                 BVH_REAL& toc)
{
  BVHModel<RSS> m1;
  BVHModel<RSS> m2;

  m1.bv_splitter.reset(new BVSplitter<RSS>(split_method));
  m2.bv_splitter.reset(new BVSplitter<RSS>(split_method));

  m1.beginModel();
  m1.addSubModel(vertices1, triangles1);
  m1.endModel();

  m2.beginModel();
  m2.addSubModel(vertices2, triangles2);
  m2.endModel();

  Vec3f R2[3];
  Vec3f T2;
  R2[0] = Vec3f(1, 0, 0);
  R2[1] = Vec3f(0, 1, 0);
  R2[2] = Vec3f(0, 0, 1);

  std::vector<Contact> contacts;

  Vec3f m1_ref;
  Vec3f m2_ref;

  if(use_COM)
  {
    for(unsigned int i = 0; i < vertices1.size(); ++i)
      m1_ref += vertices1[i];
    m1_ref *= (1.0 / vertices1.size());

    for(unsigned int i = 0; i < vertices2.size(); ++i)
      m2_ref += vertices2[i];
    m2_ref *= (1.0 / vertices2.size());
  }

  int b = conservativeAdvancement(&m1, tf1.R, tf1.T, tf2.R, tf2.T, m1_ref,
                          &m2, R2, T2, R2, T2, m2_ref,
                          1, false, false, contacts, toc);

  return (b > 0);
}


bool interp_ccd_Test(const Transform& tf1, const Transform& tf2,
                     const std::vector<Vec3f>& vertices1, const std::vector<Triangle>& triangles1,
                     const std::vector<Vec3f>& vertices2, const std::vector<Triangle>& triangles2,
                     SplitMethodType split_method,
                     unsigned int nsamples,
                     bool use_COM,
                     BVH_REAL& toc)
{

  BVHModel<RSS> m1;
  BVHModel<RSS> m2;

  m1.bv_splitter.reset(new BVSplitter<RSS>(split_method));
  m2.bv_splitter.reset(new BVSplitter<RSS>(split_method));

  m1.beginModel();
  m1.addSubModel(vertices1, triangles1);
  m1.endModel();

  m2.beginModel();
  m2.addSubModel(vertices2, triangles2);
  m2.endModel();

  Vec3f R2[3];
  Vec3f T2;
  R2[0] = Vec3f(1, 0, 0);
  R2[1] = Vec3f(0, 1, 0);
  R2[2] = Vec3f(0, 0, 1);

  Vec3f m1_ref;
  Vec3f m2_ref;

  if(use_COM)
  {
    for(unsigned int i = 0; i < vertices1.size(); ++i)
      m1_ref += vertices1[i];
    m1_ref *= (1.0 / vertices1.size());

    for(unsigned int i = 0; i < vertices2.size(); ++i)
      m2_ref += vertices2[i];
    m2_ref *= (1.0 / vertices2.size());
  }

  InterpMotion<RSS> motion1(tf1.R, tf1.T, tf2.R, tf2.T, m1_ref);

  for(unsigned int i = 0; i <= nsamples; ++i)
  {
    BVH_REAL curt = i / (BVH_REAL)nsamples;

    Vec3f R[3];
    Vec3f T;
    motion1.integrate(curt);
    motion1.getCurrentTransform(R, T);

    m1.setTransform(R, T);
    m2.setTransform(R2, T2);

    MeshCollisionTraversalNodeRSS node;
    if(!initialize(node, (const BVHModel<RSS>&)m1, (const BVHModel<RSS>&)m2))
      std::cout << "initialize error" << std::endl;

    node.enable_statistics = false;
    node.num_max_contacts = 1;
    node.exhaustive = false;
    node.enable_contact = false;

    collide(&node);

    if(node.pairs.size() > 0)
    {
      toc = curt;
      return true;
    }
  }

  return false;
}