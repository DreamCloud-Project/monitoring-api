from datetime import datetime
from mfapi import ProfilingAPI
from time import sleep
from random import randint
from socket import gethostname
from logging import INFO

def send_metric_data(workflow_id, experiment_id, task_id, platform_id):
  api.set_host(gethostname())
  api.set_platform(platform_id)

  for i in range(0, 10):
    api.update(
      workflow_id,
      experiment_id,
      {
        '@timestamp': datetime.strftime(datetime.now(), '%Y-%m-%dT%H:%M:%S.%f')[:-3],
        'energy': randint(40, 140),
        'execution_time': randint(10, 90)
      },
      task_id
    )
    sleep(1)


debuglevel = INFO

# (1) INITIALIZE API
api = ProfilingAPI('http://localhost:3030', debuglevel);

# (2) REGISTER A NEW EXPERIMENT --> EXPERIMENT ID
workflow_id = 'rm_stream'
task_ids = [ 'task1', 'task2' ]
platforms = [ 'embedded', 'hpc' ]

experiment_id = api.new_experiment(
  workflow_id,  # rm_stream
  {
    'description': 'Streaming application',
    'optimization': 'Time',
    'author': 'Raj Patel',
    'tasks': [
      {
        'name': 'task1',
        'exec': '/home/ubuntu/task1.sh',
        'cores_nr': '1-2'
      },
      {
        'name': 'task2',
        'exec': '/home/ubuntu/task2.sh',
        'cores_nr': '1-4'
      }
    ]
  }
)

# (3) ADD DEPLOYMENT PLANS FOR EXPERIMENT --> DEPLOYMENT ID
deployment_id_task1 = api.add_deployment_plan(
  workflow_id,   # rm_stream
  task_ids[0],   # task_id1
  platforms[0],  # embedded
  experiment_id, # ASDasdkjas123314sa
  {
    'estimatedTime': 217,
    'node': {
      'id': 'embedded_device',
      'cpus': [
        {
          'id': 'cpu0',
          'cores': [
            {
              'id': 'core0',
              'pwMode': 100
            },
            {
              'id': 'core1',
              'pwMode': 100
            }
          ]
        }
      ]
    }
  }
)

deployment_id_task2 = api.add_deployment_plan(
  workflow_id,   # rm_stream
  task_ids[1],   # task_id2
  platforms[1],  # hpc
  experiment_id, # ASDnasdaj12981jklADsa
  {
    'estimatedTime': 217,
    'node': {
      'id': 'embedded_device',
      'cpus': [
        {
          'id': 'cpu0',
          'cores': [
            {
              'id': 'core0',
              'pwMode': 100
            },
            {
              'id': 'core1',
              'pwMode': 100
            },
            {
              'id': 'core2',
              'pwMode': 50
            },
            {
              'id': 'core3',
              'pwMode': 50
            }
          ]
        }
      ]
    }
  }
);

# (4) SENDING METRIC DATA; BOUND TO WORKFLOW_ID, EXPERIMENT_ID, TASK_ID
send_metric_data(workflow_id, experiment_id, task_ids[0], platforms[0]);
send_metric_data(workflow_id, experiment_id, task_ids[1], platforms[1]);

