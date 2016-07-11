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
        'execution_time': randint(10, 90),
        'platform': platform_id,
        'info_channel': 'BBC1'
      },
      task_id
    )
    sleep(1)


debuglevel = INFO

# (1) INITIALIZE API
api = ProfilingAPI('http://mf.excess-project.eu:3030', debuglevel);

# (2) REGISTER A NEW EXPERIMENT --> EXPERIMENT ID
workflow_id = 'rm_stream'
experiment_id = 'Raj-20160708-21510002'
task_ids = [ 'iphone' ]
platforms = [ 'embedded', 'hpc' ]

experiment_id = api.register_experiment(
  workflow_id,  # rm_stream,
  experiment_id,
  {
    'application': 'Streaming application',
    'author': 'Raj Patel',
    'task': 'iphone'
  }
)

# (3) ADD DEPLOYMENT PLANS FOR EXPERIMENT --> DEPLOYMENT ID
deployment_id_iphone = api.add_deployment_plan(
  workflow_id,   # rm_stream
  task_ids[0],   # iphone
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

# (4) SENDING METRIC DATA; BOUND TO WORKFLOW_ID, EXPERIMENT_ID, TASK_ID
send_metric_data(workflow_id, experiment_id, task_ids[0], platforms[0]);
send_metric_data(workflow_id, experiment_id, task_ids[0], platforms[1]);

